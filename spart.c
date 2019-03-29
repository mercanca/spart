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


int spart_usage()
{
      	printf ("Usage: spart [-m]\n\n");
      	printf ("This program shows brief partition info with core count of available nodes and pending jobs.\n");
	printf ("The RESOURCE PENDING column shows core counts of pending jobs because of the busy resource.\n");
	printf ("The OTHER PENDING column shows core counts of pending jobs because of the other reasons such\n as license or other limits.\n");
	printf ("The NODE MEM(GB) column shows the memory of the lowest memory node in this partition.\n");
	printf ("If -m parameter was given, both the lowest and highest memory will be shown.\n");
      	exit (1);
}

int main (int argc, char *argv[])
{

	unsigned int i, j;
	int k;
	partition_info_msg_t *part_buffer_ptr = NULL;
	partition_info_t *part_ptr;
	node_info_msg_t *node_buffer_ptr = NULL;
	unsigned int day,hour,minute;
	unsigned int mem, cpus, min_mem, max_mem, min_cpu, free_cpu, free_node;
	unsigned int *pWaiting, *pWaitother;

	job_info_msg_t *job_buffer_ptr = NULL;

	uint16_t alloc_cpus = 0;
	char  *reason, mem_result[256];
	uint32_t state;
	int show_max_mem=0; 


  	if (argc != 1)
    	{
		if (strncmp(argv[1],"-m",3)!=0)
		{
		spart_usage();
		}
		else
		{
		show_max_mem=1;
		}
    	}


	if ( slurm_load_jobs ((time_t) NULL, &job_buffer_ptr, SHOW_ALL) ) 
	{
		slurm_perror ("slurm_load_jobs error");
		exit (1);
	}

	if ( slurm_load_node ((time_t) NULL,
					  &node_buffer_ptr, SHOW_ALL) ) {
		slurm_perror ("slurm_load_node error");
		exit (1);
	}

	if ( slurm_load_partitions ((time_t) NULL,
						   &part_buffer_ptr, 0) ) {
		slurm_perror ("slurm_load_partitions error");
		exit (1);
	}
	pWaiting=(unsigned int *) malloc(part_buffer_ptr->record_count * sizeof(unsigned int));
	pWaitother=(unsigned int *) malloc(part_buffer_ptr->record_count * sizeof(unsigned int));

	
	for (i = 0; i < part_buffer_ptr->record_count; i++) {
		pWaiting[i]=0;
		pWaitother[i]=0;
	}
	
	for (i = 0; i < job_buffer_ptr->record_count; i++) {
		for (j = 0; j < part_buffer_ptr->record_count; j++){
  			if ((strlen(part_buffer_ptr->partition_array[j].name)==strlen(job_buffer_ptr->job_array[i].partition)) &&
				 (strncmp(part_buffer_ptr->partition_array[j].name,job_buffer_ptr->job_array[i].partition,
       					strlen (part_buffer_ptr->partition_array[j].name)) == 0)) {
				if (job_buffer_ptr->job_array[i].job_state == JOB_PENDING ) {
					if ((job_buffer_ptr->job_array[i].state_reason == WAIT_RESOURCES )||
					(job_buffer_ptr->job_array[i].state_reason == WAIT_PRIORITY ))
						pWaiting[j]+=job_buffer_ptr->job_array[i].num_cpus;
					else
						pWaitother[j]+=job_buffer_ptr->job_array[i].num_cpus;
				}
			}
			/*else
			{
			printf("%s != %s\n",part_buffer_ptr->partition_array[j].name,job_buffer_ptr->job_array[i].partition);
			}*/
		}
	}
	

	printf("       QUEUE     FREE    TOTAL     FREE    TOTAL RESOURCE    OTHER   MIN   MAX  MAXJOBTIME   CORES    NODE\n");
	printf("   PARTITION    CORES    CORES    NODES    NODES  PENDING  PENDING NODES NODES   DAY-HR:MN PERNODE  MEM(GB)\n");
	for (i = 0; i < part_buffer_ptr->record_count; i++) {
		part_ptr = &part_buffer_ptr->partition_array[i];
		day=part_ptr->max_time/1440;
		hour=(part_ptr->max_time-(day*1440))/60;
		minute=part_ptr->max_time-(day*1440)-(hour*60);
		min_mem=UINT_MAX;
		max_mem=0;
		min_cpu=UINT_MAX;
		free_cpu=0;
		free_node=0;
		alloc_cpus=0;
		for (j = 0; part_ptr->node_inx; j+=2) {
			if (part_ptr->node_inx[j] == -1)
				break;
			for (k = part_ptr->node_inx[j]; k <= part_ptr->node_inx[j+1];k++)
			{
				cpus=node_buffer_ptr->node_array[k].cpus;
				mem=(unsigned int) (node_buffer_ptr->node_array[k].real_memory);
				if (min_mem>mem) min_mem=mem;
				if (max_mem<mem) max_mem=mem;
				if (min_cpu>cpus) min_cpu=cpus;

				slurm_get_select_nodeinfo(node_buffer_ptr->node_array[k].select_nodeinfo,
					SELECT_NODEDATA_SUBCNT, NODE_STATE_ALLOCATED, &alloc_cpus);
		    		state=node_buffer_ptr->node_array[k].node_state;
		    		reason=node_buffer_ptr->node_array[k].reason;
				/* The PowerSave_PwrOffState and PwrON_State_PowerSave control 
 				* for an alternative power saving solution we developed. 
 				* It required for showing power-off nodes as idle */
				if ((((state & NODE_STATE_DRAIN)!= NODE_STATE_DRAIN )&&((state==NODE_STATE_DOWN) != NODE_STATE_DOWN ))||(strncmp(reason,"PowerSave_PwrOffState",21)==0)||(strncmp(reason,"PwrON_State_PowerSave",21)==0))
				{
					if (alloc_cpus ==0) free_node+=1;
					free_cpu+=cpus-alloc_cpus;
				}
			}
		}
		printf ("%12s %8d %8d %8d %8d %8d %8d %5d ", part_ptr->name, free_cpu,part_ptr->total_cpus, free_node, part_ptr->total_nodes, pWaiting[i], pWaitother[i], part_ptr->min_nodes);
		if (part_ptr->max_nodes == UINT_MAX ) printf ("    - "); else printf ("%5d ",part_ptr->max_nodes);
		if (part_ptr->max_time == INFINITE ) printf ("   NO-LIMIT"); else  printf (" %4d-%02d:%02d",day,hour,minute);
		printf (" %7d",min_cpu);
		if ((show_max_mem == 1)&&(min_mem != max_mem )) sprintf (mem_result,"%d-%d",min_mem/1000,max_mem/1000); else  sprintf (mem_result,"%7d",min_mem/1000);
		printf (" %7s\n",mem_result);
	}


	free(pWaiting);
	free(pWaitother);
	slurm_free_job_info_msg (job_buffer_ptr);
	slurm_free_node_info_msg (node_buffer_ptr);
	slurm_free_partition_info_msg (part_buffer_ptr);
	exit (0);
}
