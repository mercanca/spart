/******************************************************************
* spart    : a user-oriented partition info command for slurm
* Author   : Cem Ahmet Mercan
* Licence  : GNU General Public License v2.0
* FirstVer.: 2019
* Note     : Some part of this code taken from slurm api man pages
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <slurm/slurm.h>
#include <slurm/slurm_errno.h>


/* Prints Command Usage and Exit */
int spart_usage() 
{
  printf("Usage: spart [-m]\n\n");
  printf(
      "This program shows brief partition info with core count of available "
      "nodes and pending jobs.\n\n");
  printf(
      "The RESOURCE PENDING column shows core counts of pending jobs "
      "because of the busy resource.\n");
  printf(
      "The OTHER PENDING column shows core counts of pending jobs because "
      "of the other reasons such\n as license or other limits.\n\n");
  printf(
      "The CORES PERNODE column shows the core count of the node with "
      "lowest core count in this partition.\n");
  printf(
      "The NODE MEM(GB) column shows the memory of the lowest memory node "
      "in this partition.\n\n");
  printf(
      "If the -m parameter was given, both the lowest and highest values will"
      " be shown in the CORES\n PERNODE and NODE MEM(GB) columns.\n\n");
  exit(1);
}


/* #define SPART_COMPILE_FOR_UHEM */

#define SPART_INFO_STRING_SIZE 256

/* To store partition info */
typedef struct spart_info 
{
    uint32_t  free_cpu;
    uint32_t  total_cpu;
    uint32_t  free_node;
    uint32_t  total_node;
    uint32_t  waiting_resourse;
    uint32_t  waiting_other;
    uint32_t  min_node;
    uint32_t  max_node;
    /* MaxJobTime */
    uint32_t  mjt_time;
    uint16_t  mjt_day;
    uint16_t  mjt_hour;
    uint16_t  mjt_minute;
    uint16_t  min_core;
    uint16_t  max_core;
    uint16_t  min_mem_gb;
    uint16_t  max_mem_gb;
    char      partition_name[SPART_INFO_STRING_SIZE];
} spart_info_t; 



int main(int argc, char *argv[])
{
  uint32_t i, j;
  int k;
  partition_info_msg_t *part_buffer_ptr = NULL;
  partition_info_t *part_ptr;
  node_info_msg_t *node_buffer_ptr = NULL;
  uint32_t mem, cpus, min_mem, max_mem;
  uint32_t max_cpu, min_cpu, free_cpu, free_node;

  job_info_msg_t *job_buffer_ptr = NULL;

  uint16_t alloc_cpus = 0;
#ifdef SPART_COMPILE_FOR_UHEM
  char *reason;
#endif
  char mem_result[SPART_INFO_STRING_SIZE];
  uint32_t state;
  int show_max_mem = 0;

  char partition_str[SPART_INFO_STRING_SIZE];
  char job_parts_str[SPART_INFO_STRING_SIZE];


  spart_info_t *spData = NULL;
  uint32_t partition_count = 0;

  if (argc != 1)
  {
    if (strncmp(argv[1], "-m", 3) != 0)
    {
      spart_usage();
    } else 
    {
      show_max_mem = 1;
    }
  }

  if (slurm_load_jobs((time_t)NULL, &job_buffer_ptr, SHOW_ALL))
  {
    slurm_perror("slurm_load_jobs error");
    exit(1);
  }

  if (slurm_load_node((time_t)NULL, &node_buffer_ptr, SHOW_ALL))
  {
    slurm_perror("slurm_load_node error");
    exit(1);
  }

  if (slurm_load_partitions((time_t)NULL, &part_buffer_ptr, 0))
  {
    slurm_perror("slurm_load_partitions error");
    exit(1);
  }


  /* Initialize spart data for each partition */
  partition_count=part_buffer_ptr->record_count;
  spData = (spart_info_t *)malloc(partition_count * sizeof(spart_info_t));

  for (i = 0; i < partition_count; i++)
  {
    spData[i].free_cpu = 0;
    spData[i].total_cpu = 0;
    spData[i].free_node = 0;
    spData[i].total_node = 0;
    spData[i].waiting_resourse = 0;
    spData[i].waiting_other = 0;
    spData[i].min_node = 0;
    spData[i].max_node = 0;
    /* MaxJobTime */
    spData[i].mjt_time = 0;
    spData[i].mjt_day = 0;
    spData[i].mjt_hour = 0;
    spData[i].mjt_minute = 0;
    spData[i].min_core = 0;
    spData[i].max_core = 0;
    spData[i].min_mem_gb = 0;
    spData[i].max_mem_gb = 0;
    /* partition_name[] */
  }




  /* Finds resourse/other waiting core count for each partition */
  for (i = 0; i < job_buffer_ptr->record_count; i++)
  {
    strncpy(job_parts_str,job_buffer_ptr->job_array[i].partition,SPART_INFO_STRING_SIZE);
    /* and ',' character at the end */ 
    strncat(job_parts_str+strlen(job_parts_str),",",SPART_INFO_STRING_SIZE);
    for (j = 0; j < partition_count; j++)
    {
      
      strncpy(partition_str,part_buffer_ptr->partition_array[j].name,SPART_INFO_STRING_SIZE);
      /* and ',' character at the end */ 
      strncat(partition_str+strlen(partition_str),",",SPART_INFO_STRING_SIZE);
      if(strstr(job_parts_str, partition_str) != NULL)
      {
        if (job_buffer_ptr->job_array[i].job_state == JOB_PENDING)
        {
          if ((job_buffer_ptr->job_array[i].state_reason == WAIT_RESOURCES) ||
              (job_buffer_ptr->job_array[i].state_reason == WAIT_PRIORITY))
            spData[j].waiting_resourse += job_buffer_ptr->job_array[i].num_cpus;
          else
            spData[j].waiting_other += job_buffer_ptr->job_array[i].num_cpus;
        }
      }
    }
  }



  for (i = 0; i < partition_count; i++)
  {
    part_ptr = &part_buffer_ptr->partition_array[i];

    min_mem = UINT_MAX;
    max_mem = 0;
    min_cpu = UINT_MAX;
    max_cpu = 0;
    free_cpu = 0;
    free_node = 0;
    alloc_cpus = 0;
    for (j = 0; part_ptr->node_inx; j += 2) 
    {
      if (part_ptr->node_inx[j] == -1) break;
      for (k = part_ptr->node_inx[j]; k <= part_ptr->node_inx[j + 1]; k++)
      {
        cpus = node_buffer_ptr->node_array[k].cpus;
        mem = (uint32_t)(node_buffer_ptr->node_array[k].real_memory);
        if (min_mem > mem) min_mem = mem;
        if (max_mem < mem) max_mem = mem;
        if (min_cpu > cpus) min_cpu = cpus;
        if (max_cpu < cpus) max_cpu = cpus;

        slurm_get_select_nodeinfo(
            node_buffer_ptr->node_array[k].select_nodeinfo,
            SELECT_NODEDATA_SUBCNT, NODE_STATE_ALLOCATED, &alloc_cpus);

        state = node_buffer_ptr->node_array[k].node_state;
#ifdef SPART_COMPILE_FOR_UHEM
        reason = node_buffer_ptr->node_array[k].reason;
#endif

        /* The PowerSave_PwrOffState and PwrON_State_PowerSave control
        * for an alternative power saving solution we developed.
        * It required for showing power-off nodes as idle */
        if ((((state & NODE_STATE_DRAIN) != NODE_STATE_DRAIN) &&
             ((state == NODE_STATE_DOWN) != NODE_STATE_DOWN))
#ifdef SPART_COMPILE_FOR_UHEM
             || (strncmp(reason, "PowerSave_PwrOffState", 21) == 0) 
             || (strncmp(reason, "PwrON_State_PowerSave", 21) == 0)
#endif
             )
        {
          if (alloc_cpus == 0) free_node += 1;
          free_cpu += cpus - alloc_cpus;
        }
      }
    }

    spData[i].free_cpu=free_cpu;
    spData[i].total_cpu=part_ptr->total_cpus;
    spData[i].free_node=free_node;
    spData[i].total_node=part_ptr->total_nodes;
    /* spData[i].waiting_resourse and spData[i].waiting_other previously set */
    spData[i].min_node=part_ptr->min_nodes;
    spData[i].max_node=part_ptr->max_nodes;
    spData[i].mjt_time = part_ptr->max_time;
    spData[i].mjt_day = part_ptr->max_time / 1440;
    spData[i].mjt_hour = (part_ptr->max_time - (spData[i].mjt_day * 1440)) / 60;
    spData[i].mjt_minute = part_ptr->max_time - (spData[i].mjt_day * 1440) - (spData[i].mjt_hour * 60);
    spData[i].min_core=min_cpu;
    spData[i].max_core=max_cpu;
    spData[i].max_mem_gb=(uint16_t)(max_mem/1000u);
    spData[i].min_mem_gb=(uint16_t)(min_mem/1000u);
    strncpy(spData[i].partition_name,part_ptr->name,SPART_INFO_STRING_SIZE);
  }


  /* Output is printing */
  printf(
      "       QUEUE     FREE    TOTAL     FREE    TOTAL RESOURCE    "
      "OTHER   MIN   MAX MAXJOBTIME   CORES    NODE\n");
  printf(
      "   PARTITION    CORES    CORES    NODES    NODES  PENDING  "
      "PENDING NODES NODES  DAY-HR:MN PERNODE MEM(GB)\n");

  for (i = 0; i < partition_count; i++)
  {
    printf("%12s %8d %8d %8d %8d %8d %8d %5d ", spData[i].partition_name, 
           spData[i].free_cpu, spData[i].total_cpu, spData[i].free_node, 
           spData[i].total_node, spData[i].waiting_resourse,
           spData[i].waiting_other, spData[i].min_node);
    if (spData[i].max_node == UINT_MAX)
      printf("    - ");
    else
      printf("%5d ", spData[i].max_node);
    if (spData[i].mjt_time == INFINITE)
      printf("  NO-LIMIT");
    else
      printf("%4d-%02d:%02d", spData[i].mjt_day, spData[i].mjt_hour, spData[i].mjt_minute);
    if ((show_max_mem == 1)&&(spData[i].min_core != spData[i].max_core )) 
      sprintf (mem_result,"%d-%d",spData[i].min_core,spData[i].max_core); 
    else  
      sprintf (mem_result,"%7d",spData[i].min_core);
    printf (" %7s",mem_result);
    if ((show_max_mem == 1)&&(spData[i].min_mem_gb != spData[i].max_mem_gb )) 
      sprintf (mem_result,"%d-%d",spData[i].min_mem_gb,spData[i].max_mem_gb); 
    else  
      sprintf (mem_result,"%7d",spData[i].min_mem_gb);
    printf (" %7s\n",mem_result);
  }

  /* free allocations */
  free(spData);
  slurm_free_job_info_msg(job_buffer_ptr);
  slurm_free_node_info_msg(node_buffer_ptr);
  slurm_free_partition_info_msg(part_buffer_ptr);
  exit(0);
}
