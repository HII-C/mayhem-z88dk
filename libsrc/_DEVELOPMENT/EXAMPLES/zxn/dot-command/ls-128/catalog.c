#include <string.h>
#include <libgen.h>
#include <arch/zxn/esxdos.h>
#include <alloc/obstack.h>
#include <adt/p_queue.h>

#include "catalog.h"
#include "list.h"
#include "ls.h"
#include "memory.h"

#include <stdio.h>

unsigned char catalog_control;
unsigned char catalog_morethanone;

// lfn & catalog must be available to NextZXOS +3e api
// which means they must lie inside [0x4000,0xbfe0)

struct esx_lfn lfn;
struct esx_cat catalog;


static struct dir_record *first;

void catalog_add_dir_record(unsigned char *name)
{
   int size;

   static struct dir_record *last;
   static struct dir_record *next;
   static struct dir_record *spot;
   
   // name is in main bank
   // must already end in pattern "/*.*"
   
   // page in directory bank
   
   memory_page_in_dir(BASE_DIR_PAGES);
   
   // compute size of directory entry needed
   // (space for zero terminator built into struct dir_record)
   
   size = strlen(name) + sizeof(struct dir_record);
   
   // allocate a spot in directory queue if we can
   
   spot = 0;
   
   if ((first = p_queue_front(&dqueue)) == 0)
   {
      // the directory queue is empty
      // assume one directory entry will always fit
      
      spot = (struct dir_record *)dir_start;
   }
   else
   {
      // the directory queue is not empty
      
      last = p_queue_back(&dqueue);
      next = (struct dir_record *)((unsigned char *)last + last->size);
      
      if (first <= last)
      {         
         if ((next >= last) && (((unsigned char *)dir_end - (unsigned char *)next) >= size))
            spot = next;
      }
      
      if (spot == 0)
      {
         if (first <= last)
         {
            next = (struct dir_record *)dir_start;
            last = next;
         }
         
         if ((next >= last) && (((unsigned char *)first - (unsigned char *)next) >= size))
            spot = next;
      }
   }

   // silently ignore if no space is available
   
   if (spot)
   {
      spot->size = size;
      strcpy(spot->name, name);
      
      p_queue_push(&dqueue, spot);
   }

   // restore main bank
   
   memory_restore_dir();

   return;
}

static struct file_record_ptr frp;
static struct file_record fr;

unsigned char catalog_add_file_record(void)
{
   static unsigned int size;
   
   size = strlen(lfn.filename) + sizeof(fr);
   
   // locate memory to store file record
   
   {
      unsigned char i;
      
      for (i = 0; i != NUM_LFN_PAGES; ++i)
      {
         memory_page_in_mmu7(clob + BASE_LFN_PAGES);
      
         if (frp.fr = obstack_alloc(lob[clob], size))
            break;
      
         if (++clob >= NUM_LFN_PAGES)
            clob = 0;
      }
   
      if (i == NUM_LFN_PAGES) return 1;
   }
   
   frp.page = clob;
   
   // complete fr
   
   fr.len = 0;
   
   if (flags.name_fmt_mod & FLAG_NAME_FMT_MOD_LFN)
      fr.len = size - sizeof(fr);
   
   if (flags.name_fmt_mod & FLAG_NAME_FMT_MOD_SFN)
      fr.len += 12;
   
   if (flags.name_fmt_mod & (FLAG_NAME_FMT_MOD_LFN | FLAG_NAME_FMT_MOD_SFN))
      fr.len += 2;
   
   // copy completed file record to obstack
   
   memcpy(frp.fr, &fr, sizeof(fr));
   memcpy(frp.fr->lfn, lfn.filename, size - sizeof(fr) + 1);

//printf("lfn size = %lu\n", lfn.size);
//printf("fr size = %lu\n", frp.fr->size);
   // restore memory
   
   memory_restore_mmu7();
   
   // add file record pointer
   
   if (obstack_copy(fob, &frp, sizeof(frp)))
   {
      ++fbase_sz;
      return 0;
   }
   
   return 1;
}

static unsigned char name_in_main_memory[ESX_FILENAME_LFN_MAX + 1];
static unsigned char constructed_name[ESX_FILENAME_LFN_MAX*2 + 1];

void catalog_add_file_records(unsigned char *name)
{
   unsigned char another;
      
   static unsigned char morethanone;
   static unsigned char enter_dir;
   static unsigned char dots;
   static unsigned char filter;
   
   static unsigned char *constructed_name_base;

   // name is visible and could be in divmmc or dir bank
   // main bank is restored in mmu6,7 by this function
   
   strlcpy(name_in_main_memory, name, sizeof(name_in_main_memory));
   strcpy(constructed_name, name_in_main_memory);
   
   constructed_name_base = advance_past_drive(constructed_name);
   if (*constructed_name_base) constructed_name_base = basename(constructed_name_base);

   // restore mmu6,7
   
   memory_restore_dir();
   
   // init catalog structure for matching
   
   catalog.filter = (flags.sys) ? (ESX_CAT_FILTER_SYSTEM | ESX_CAT_FILTER_LFN | ESX_CAT_FILTER_DIR) : (ESX_CAT_FILTER_LFN | ESX_CAT_FILTER_DIR);
   catalog.filename = p3dos_cstr_to_pstr(name_in_main_memory);
   catalog.cat_sz = 2;
   
   lfn.cat = &catalog;
   
   // iterate over all matches
   
   if (esx_dos_catalog(&catalog) == 1)
   {
      enter_dir = (catalog_control == CATALOG_MODE_SRC) && (flags.dir_type != FLAG_DIR_TYPE_LIST);

      do
      {
         // lfn details for this file

         esx_ide_get_lfn(&lfn, &catalog.cat[1]);

         // fill in matched file details
         
         fr.type = (catalog.cat[1].filename[7] & 0x80) ? FILE_RECORD_TYPE_DIR : FILE_RECORD_TYPE_FILE;
         memcpy(&fr.time, &lfn.time, sizeof(fr.time) + sizeof(fr.size));
         p3dos_dosname_from_catname(fr.sfn, catalog.cat[1].filename);

         // find out if there is another match
         
         another = (esx_dos_catalog_next(&catalog) == 1);

         // filter listed files

         dots = (strcmp(lfn.filename, ".") == 0) || (strcmp(lfn.filename, "..") == 0);
         
         if ((filter = dots && (flags.file_filter & FLAG_FILE_FILTER_ALMOST_ALL)) == 0)
            filter = (flags.file_filter & FLAG_FILE_FILTER_BACKUP) && (stricmp(basename_ext(lfn.filename), ".bak") == 0);
         
         if (filter == 0)
         {
            // indicate if there is more than one match

            catalog_morethanone |= another;

            // if there's one match and it's a directory we may want to enter the directory instead of listing it
         
            if ((enter_dir == 0) || (fr.type != FILE_RECORD_TYPE_DIR) || catalog_morethanone)
            {
               // file is part of list
            
               if (catalog_add_file_record())
               {
                  // out of memory for records so must list partially complete directory
               
                  list_generate();
                  memory_clear_file_records();
               
                  // now add record
               
                  catalog_add_file_record();
               }
            }
         
            // determine if we're adding directory to queue
         
            if ((fr.type == FILE_RECORD_TYPE_DIR) && (dots == 0) && ((flags.dir_type == FLAG_DIR_TYPE_RECURSIVE) || (enter_dir && (catalog_morethanone == 0))))
            {
               // construct name

               strcat(strcpy(constructed_name_base, lfn.filename), "/*.*");

               // add directory to queue

               catalog_add_dir_record(constructed_name);
            }
         }
      }
      while (another == 1);
   }
   
   return;
}
