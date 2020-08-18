/******************************************************************
 * spart    : a user-oriented partition info command for slurm
 * Author   : Cem Ahmet Mercan, 2019-02-16
 * Licence  : GNU General Public License v2.0
 * Note     : Some part of this code taken from slurm api man pages
 *******************************************************************/

#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <slurm/slurm.h>
#include <slurm/slurm_errno.h>
#include <slurm/slurmdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* for UHeM-ITU-Turkey specific settings */
/* #define SPART_COMPILE_FOR_UHEM */

/* #define SPART_SHOW_STATEMENT */

/* if you want to use STATEMENT feature, uncomment
 * SPART_SHOW_STATEMENT at upper line, and set
 * SPART_STATEMENT_DIR to show the correct directory
 * which will be saved all statement files.
 * Write your statement into statement file which
 * SPART_STATEMENT_DIR/SPART_STATEMENT_FILE
 * as a regular text, but max 91 chars per line.
 * SPART_STATEMENT_DIR and SPART_STATEMENT_FILE
 * should be readable by everyone.
 * You can change or remove STATEMENT file without
 * recompiling the spart. If the spart find a
 * statement file, it will show your statement. */
#ifdef SPART_SHOW_STATEMENT

#define SPART_STATEMENT_DIR "/okyanus/SLURM/spart/"
#define SPART_STATEMENT_FILE "spart_STATEMENT.txt"

/* if you want to show a bold STATEMENT, define like these
 * #define SPART_STATEMENT_LINEPRE "\033[1m"
 * #define SPART_STATEMENT_LINEPOST "\033[0m"
 */
/* for the white text over the red background
 * #define SPART_STATEMENT_LINEPRE "\033[37;41;1m"
 * #define SPART_STATEMENT_LINEPOST "\033[0m"
 */
/* and, for regular statement text, use these
#define SPART_STATEMENT_LINEPRE ""
#define SPART_STATEMENT_LINEPOST ""
*/
#define SPART_STATEMENT_LINEPRE ""
#define SPART_STATEMENT_LINEPOST ""

/* Same as SPART_STATEMENT_LINEPRE,
 * but these for QUEUE STATEMENTs */
#define SPART_STATEMENT_QUEUE_LINEPRE ""
#define SPART_STATEMENT_QUEUE_LINEPOST ""

/* The partition statements will be located at
 * SPART_STATEMENT_DIR/
 * SPART_STATEMENT_QUEPRE<partition>SPART_STATEMENT_QUEPOST
 * and will be shown, if -i parameter is given.
 */
#define SPART_STATEMENT_QUEPRE "spart_partition_"
#define SPART_STATEMENT_QUEPOST ".txt"
#endif

#define SPART_INFO_STRING_SIZE 4096
#define SPART_GRES_ARRAY_SIZE 256
#define SPART_MAX_COLUMN_SIZE 64
#define SPART_MAX_GROUP_SIZE 32

/* Prints Command Usage and Exit */
int spart_usage() {
  printf(
      "\nUsage: spart [-m] [-a] "
#ifdef __slurmdb_cluster_rec_t_defined
      "[-c] "
#endif
      "[-g] [-i] [-t] [-f] [-l] [-s] [-J] [-h]\n\n");
  printf(
      "This program shows brief partition info with core count of available "
      "nodes and pending jobs.\n\n");
  printf(
      "In the STA-TUS column, the characters means, the partition is:\n"
      "\t*\tdefault partition (default queue),\n"
      "\t.\thidden partition,\n"
      "\tC\tclosed to both the job submit and run,\n"
      "\tS\tclosed to the job submit, but the submitted jobs will run,\n"
      "\tr\trequires the reservation,\n"
      /* "\tx\tthe exclusive job permitted,\n" */
      "\tD\topen to the job submit, but the submitted jobs will not run,\n"
      "\tR\topen for only root, or closed to root (if you are root),\n"
      "\tA\tclosed to all of your account(s),\n"
      "\ta\tclosed to some of your account(s),\n"
      "\tG\tclosed to all of your group(s),\n"
      "\tg\tclosed to some of your group(s),\n"
      "\tQ\tclosed to all of your QOS(s),\n"
      "\tq\tclosed to some of your QOS(s).\n\n");
  printf(
      "The RESOURCE PENDING column shows core counts of pending jobs "
      "because of the busy resource.\n\n");
  printf(
      "The OTHER PENDING column shows core counts of pending jobs because "
      "of the other reasons such\n as license or other limits.\n\n");
  printf(
      "The YOUR-RUN, PEND-RES, PEND-OTHR, and YOUR-TOTL columns shows "
      "the counts of the running,\n resource pending, other pending, and "
      "total job count of the current user, respectively.\n If these four "
      "columns are have same values, These same values of that four columns "
      "will be\n shown at COMMON VALUES as four single values.\n\n");
  printf(
      "The MIN NODE and MAX NODE columns show the permitted minimum and "
      "maximum node counts of the\n jobs which can be submited to the "
      "partition.\n\n");
  printf(
      "The MAXCPU/NODE column shows the permitted maximum core counts of "
      "of the single node in\n the partition.\n\n");
  printf(
      "The DEFMEM GB/CPU and DEFMEM GB/NODE columns show default maximum "
      "memory as GB which a job\n can use for a cpu or a node, "
      "respectively.\n\n");
  printf(
      "The MAXMEM GB/CPU and MAXMEM GB/NODE columns show maximum "
      "memory as GB which requestable by\n a job for a cpu or a node, "
      "respectively.\n\n");
  printf(
      "The DEFAULT JOB-TIME column shows the default time limit of the job "
      "which submited to the\n partition without a time limit. If "
      "the DEFAULT JOB-TIME limits are not setted, or setted\n same value with "
      "MAXIMUM JOB-TIME for all partitions in your cluster, DEFAULT JOB-TIME\n "
      "column will not be shown, except -l parameter was given.\n\n");
  printf(
      "The MAXIMUM JOB-TIME column shows the maximum time limit of the job "
      "which submited to the\n partition. If the user give a time limit "
      "further than MAXIMUM JOB-TIME limit of the\n partition, the job will be "
      "rejected by the slurm.\n\n");
  printf(
      "The CORES/NODE column shows the core count of the node with "
      "lowest core count in the\n partition. But if -l was given, both "
      "the lowest and highest core counts will be shown.\n\n");
  printf(
      "The NODE MEM-GB column shows the memory of the lowest memory node "
      "in this partition. But if\n -l parameter was given, both the lowest "
      "and highest memory will be shown.\n\n");
  printf(
      "The QOS NAME column shows the default qos limits the job "
      "which submited to the partition.\n If the QOS NAME of the partition "
      "are not setted for all partitions in your cluster, QOS\n NAME column "
      "will not be shown, execpt -l parameter was given.\n\n");
  printf(
      "The GRES (COUNT) column shows the generic resources of the nodes "
      "in the partition, and (in\n paranteses) the total number of nodes "
      "in that partition containing that GRES. The GRES\n (COUNT) column "
      "will not be shown, execpt -l or -g parameter was given.\n\n");
  printf(
      "If the partition's QOS NAME, MIN NODES, MAX NODES, MAXCPU/NODE, "
      "DEFMEM GB/CPU|NODE,\n MAXMEM GB/CPU|NODE, DEFAULT JOB-TIME, and MAXIMUM "
      "JOB-TIME limits are not setted for the\n all partitions in your "
      "cluster, corresponding column(s) will not be shown, except -l\n "
      "parameter was given.\n\n");
  printf(
      "If the values of a column are same, this column will not be shown "
      "at partitions block.\n These same values of that column will be show at "
      "COMMON VALUES as a single value.\n\n");
  printf("Parameters:\n\n");
  printf(
      "\t-m\tboth the lowest and highest values will be shown in the CORES"
      " /NODE\n\t\tand NODE MEM-GB columns.\n\n");
  printf("\t-a\thidden partitions also be shown.\n\n");
#ifdef __slurmdb_cluster_rec_t_defined
  printf("\t-c\tpartitions from federated clusters be shown.\n\n");
#endif
  printf(
      "\t-g\tthe ouput shows each GRES (gpu, mic etc.) defined in that "
      "partition and\n\t\t(in paranteses) the total number of nodes in that "
      "partition containing\n\t\tthat GRES.\n\n");
  printf(
      "\t-i\tthe info about the groups, accounts, QOSs, and queues will "
      "be shown.\n\n");
  printf(
      "\t-t\tthe time info will be shown at DAY-HR:MN format, instead of "
      "verbal format.\n\n");
  printf(
      "\t-f\tthe ouput shows each FEATURES defined in that partition "
      "and (in paranteses)\n\t\tthe total number of nodes in that "
      "partition containing that FEATURES.\n\n");
  printf("\t-s\tthe simple output. spart don't show slurm config columns.\n\n");
  printf("\t-J\tthe output does not shown the info about the user's jobs.\n\n");
  printf(
      "\t-l\tall posible columns will be shown, except"
      " the federated clusters column.\n\n");
  printf("\t-h\tshows this usage text.\n\n");
#ifdef SPART_COMPILE_FOR_UHEM
  printf("This is UHeM Version of the spart command.\n");
#endif
  printf("spart version 1.3.2\n\n");
  exit(1);
}

/* To store partition info */
typedef struct spart_info {
  uint32_t free_cpu;
  uint32_t total_cpu;
  uint32_t free_node;
  uint32_t total_node;
  uint32_t waiting_resource;
  uint32_t waiting_other;
  uint32_t min_nodes;
  uint32_t max_nodes;
  uint64_t def_mem_per_cpu;
  uint64_t max_mem_per_cpu;
  uint32_t max_cpus_per_node;
  /* MaxJobTime */
  uint32_t mjt_time;
  /* DefaultJobTime */
  uint32_t djt_time;
  uint32_t min_core;
  uint32_t max_core;
  uint16_t min_mem_gb;
  uint16_t max_mem_gb;
  uint16_t visible;
#ifdef SPART_SHOW_STATEMENT
  uint16_t show_statement;
#endif
  uint32_t my_waiting_resource;
  uint32_t my_waiting_other;
  uint32_t my_running;
  uint32_t my_total;

  char partition_name[SPART_MAX_COLUMN_SIZE];
#ifdef __slurmdb_cluster_rec_t_defined
  char cluster_name[SPART_MAX_COLUMN_SIZE];
#endif
  char partition_qos[SPART_MAX_COLUMN_SIZE];
  char gres[SPART_INFO_STRING_SIZE];
  char features[SPART_INFO_STRING_SIZE];
  char partition_status[SPART_MAX_COLUMN_SIZE];
} spart_info_t;

/* To storing info about a gres */
typedef struct sp_gres_info {
  uint32_t count;
  char gres_name[SPART_INFO_STRING_SIZE];
} sp_gres_info_t;

/* Add one gres to partition gres list */
void sp_gres_add(sp_gres_info_t spga[], uint16_t *sp_gres_count,
                 char *node_gres) {
  uint16_t i;
  int found = 0;
  char *strtmp = NULL;
  char *grestok;

  for (grestok = strtok_r(node_gres, ",", &strtmp); grestok != NULL;
       grestok = strtok_r(NULL, ",", &strtmp)) {
    for (i = 0; i < *sp_gres_count; i++) {
      if (strncmp(spga[i].gres_name, grestok, SPART_INFO_STRING_SIZE) == 0) {
        spga[i].count++;
        found = 1;
        break;
      }
    }

    if (found == 0) {
      strncpy(spga[*sp_gres_count].gres_name, grestok, SPART_INFO_STRING_SIZE);
      spga[*sp_gres_count].count = 1;
      (*sp_gres_count)++;
    }
  }
}

/* Sets all counts as zero in the partition gres list */
void sp_gres_reset_counts(sp_gres_info_t *spga, uint16_t *sp_gres_count) {
  uint16_t i;
  for (i = 0; i < *sp_gres_count; i++) {
    spga[i].count = 0;
  }
  (*sp_gres_count) = 0;
}

/* Checks the user accounts present at the partition accounts */
int sp_account_check(char **useracct, int useract_count, char *partition_acct) {
  uint16_t i;
  int *found = NULL;
  int parti = 0;
  char *strtmp = NULL;
  char *grestok;

  found = malloc(useract_count * sizeof(int));
  for (i = 0; i < useract_count; i++) found[i] = 0;

  for (grestok = strtok_r(partition_acct, ",", &strtmp); grestok != NULL;
       grestok = strtok_r(NULL, ",", &strtmp)) {
    for (i = 0; i < useract_count; i++) {
      if (strncmp(useracct[i], grestok, SPART_INFO_STRING_SIZE) == 0) {
        found[i]++;
        break;
      }
    }
  }
  /* How many accounts/groups of user listed in the partition list */
  for (i = 0; i < useract_count; i++)
    if (found[i] > 0) parti++;
  free(found);
  return parti;
}

/* An output column header info */
typedef struct sp_column_header {
  char line1[SPART_INFO_STRING_SIZE];
  char line2[SPART_INFO_STRING_SIZE];
  uint16_t column_width;
  uint16_t visible;
} sp_column_header_t;

/* To storing Output headers */
typedef struct sp_headers {
  sp_column_header_t hspace;
#ifdef __slurmdb_cluster_rec_t_defined
  sp_column_header_t cluster_name;
#endif
  sp_column_header_t partition_name;
  sp_column_header_t partition_status;
  sp_column_header_t free_cpu;
  sp_column_header_t total_cpu;
  sp_column_header_t free_node;
  sp_column_header_t total_node;
  sp_column_header_t waiting_resource;
  sp_column_header_t waiting_other;
  sp_column_header_t my_running;
  sp_column_header_t my_waiting_resource;
  sp_column_header_t my_waiting_other;
  sp_column_header_t my_total;
  sp_column_header_t min_nodes;
  sp_column_header_t max_nodes;
  sp_column_header_t max_cpus_per_node;
  sp_column_header_t def_mem_per_cpu;
  sp_column_header_t max_mem_per_cpu;
  /* MaxJobTime */
  sp_column_header_t mjt_time;
  sp_column_header_t djt_time;
  sp_column_header_t min_core;
  sp_column_header_t min_mem_gb;
  sp_column_header_t partition_qos;
  sp_column_header_t gres;
  sp_column_header_t features;
} sp_headers_t;

/* Initialize all column headers */
void sp_headers_set_defaults(sp_headers_t *sph) {
  sph->hspace.visible = 0;
  sph->hspace.column_width = 17;
  strncpy(sph->hspace.line1, "           ", sph->hspace.column_width);
  strncpy(sph->hspace.line2, "           ", sph->hspace.column_width);
#ifdef __slurmdb_cluster_rec_t_defined
  sph->cluster_name.visible = 0;
  sph->cluster_name.column_width = 8;
  strncpy(sph->cluster_name.line1, " CLUSTER", sph->cluster_name.column_width);
  strncpy(sph->cluster_name.line2, "    NAME", sph->cluster_name.column_width);
#endif
  sph->partition_name.visible = 1;
  sph->partition_name.column_width = 10;
  strncpy(sph->partition_name.line1, "     QUEUE",
          sph->partition_name.column_width);
  strncpy(sph->partition_name.line2, " PARTITION",
          sph->partition_name.column_width);
  sph->partition_status.visible = 1;
  sph->partition_status.column_width = 3;
  strncpy(sph->partition_status.line1, "STA",
          sph->partition_status.column_width);
  strncpy(sph->partition_status.line2, "TUS",
          sph->partition_status.column_width);
  sph->free_cpu.visible = 1;
  sph->free_cpu.column_width = 6;
  strncpy(sph->free_cpu.line1, "  FREE", sph->free_cpu.column_width);
  strncpy(sph->free_cpu.line2, " CORES", sph->free_cpu.column_width);
  sph->total_cpu.visible = 1;
  sph->total_cpu.column_width = 6;
  strncpy(sph->total_cpu.line1, " TOTAL", sph->total_cpu.column_width);
  strncpy(sph->total_cpu.line2, " CORES", sph->total_cpu.column_width);
  sph->free_node.visible = 1;
  sph->free_node.column_width = 6;
  strncpy(sph->free_node.line1, "  FREE", sph->free_node.column_width);
  strncpy(sph->free_node.line2, " NODES", sph->free_node.column_width);
  sph->total_node.visible = 1;
  sph->total_node.column_width = 6;
  strncpy(sph->total_node.line1, " TOTAL", sph->total_node.column_width);
  strncpy(sph->total_node.line2, " NODES", sph->total_node.column_width);
  sph->waiting_resource.visible = 1;
  sph->waiting_resource.column_width = 6;
  strncpy(sph->waiting_resource.line1, "RESORC",
          sph->waiting_resource.column_width);
  strncpy(sph->waiting_resource.line2, "PENDNG",
          sph->waiting_resource.column_width);
  sph->waiting_other.visible = 1;
  sph->waiting_other.column_width = 6;
  strncpy(sph->waiting_other.line1, " OTHER", sph->waiting_other.column_width);
  strncpy(sph->waiting_other.line2, "PENDNG", sph->waiting_other.column_width);
  sph->my_running.visible = 1;
  sph->my_running.column_width = 4;
  strncpy(sph->my_running.line1, "YOUR", sph->my_running.column_width);
  strncpy(sph->my_running.line2, " RUN", sph->my_running.column_width);
  sph->my_waiting_resource.visible = 1;
  sph->my_waiting_resource.column_width = 4;
  strncpy(sph->my_waiting_resource.line1, "PEND",
          sph->my_waiting_resource.column_width);
  strncpy(sph->my_waiting_resource.line2, " RES",
          sph->my_waiting_resource.column_width);
  sph->my_waiting_other.visible = 1;
  sph->my_waiting_other.column_width = 4;
  strncpy(sph->my_waiting_other.line1, "PEND",
          sph->my_waiting_other.column_width);
  strncpy(sph->my_waiting_other.line2, "OTHR",
          sph->my_waiting_other.column_width);
  sph->my_total.visible = 1;
  sph->my_total.column_width = 4;
  strncpy(sph->my_total.line1, "YOUR", sph->my_total.column_width);
  strncpy(sph->my_total.line2, "TOTL", sph->my_total.column_width);
  sph->min_nodes.visible = 1;
  sph->min_nodes.column_width = 5;
  strncpy(sph->min_nodes.line1, "  MIN", sph->min_nodes.column_width);
  strncpy(sph->min_nodes.line2, "NODES", sph->min_nodes.column_width);
  sph->max_nodes.visible = 1;
  sph->max_nodes.column_width = 5;
  strncpy(sph->max_nodes.line1, "  MAX", sph->max_nodes.column_width);
  strncpy(sph->max_nodes.line2, "NODES", sph->max_nodes.column_width);
  sph->max_cpus_per_node.visible = 0;
  sph->max_cpus_per_node.column_width = 6;
  strncpy(sph->max_cpus_per_node.line1, "MAXCPU",
          sph->max_cpus_per_node.column_width);
  strncpy(sph->max_cpus_per_node.line2, " /NODE",
          sph->max_cpus_per_node.column_width);
  sph->max_mem_per_cpu.visible = 0;
  sph->max_mem_per_cpu.column_width = 6;
  strncpy(sph->max_mem_per_cpu.line1, "MAXMEM",
          sph->max_mem_per_cpu.column_width);
  strncpy(sph->max_mem_per_cpu.line2, "GB/CPU",
          sph->max_mem_per_cpu.column_width);
  sph->def_mem_per_cpu.visible = 0;
  sph->def_mem_per_cpu.column_width = 6;
  strncpy(sph->def_mem_per_cpu.line1, "DEFMEM",
          sph->def_mem_per_cpu.column_width);
  strncpy(sph->def_mem_per_cpu.line2, "GB/CPU",
          sph->def_mem_per_cpu.column_width);
  sph->djt_time.visible = 0;
  sph->djt_time.column_width = 10;
  strncpy(sph->djt_time.line1, "   DEFAULT", sph->djt_time.column_width);
  strncpy(sph->djt_time.line2, "  JOB-TIME", sph->djt_time.column_width);
  sph->mjt_time.visible = 1;
  sph->mjt_time.column_width = 10;
  strncpy(sph->mjt_time.line1, "   MAXIMUM", sph->mjt_time.column_width);
  strncpy(sph->mjt_time.line2, "  JOB-TIME", sph->mjt_time.column_width);
  sph->min_core.visible = 1;
  sph->min_core.column_width = 6;
  strncpy(sph->min_core.line1, " CORES", sph->min_core.column_width);
  strncpy(sph->min_core.line2, " /NODE", sph->min_core.column_width);
  sph->min_mem_gb.visible = 1;
  sph->min_mem_gb.column_width = 6;
  strncpy(sph->min_mem_gb.line1, "  NODE", sph->min_mem_gb.column_width);
  strncpy(sph->min_mem_gb.line2, "MEM-GB", sph->min_mem_gb.column_width);
  sph->partition_qos.visible = 1;
  sph->partition_qos.column_width = 6;
  strncpy(sph->partition_qos.line1, "   QOS", sph->partition_qos.column_width);
  strncpy(sph->partition_qos.line2, "  NAME", sph->partition_qos.column_width);
  sph->gres.visible = 0;
  sph->gres.column_width = 12;
  strncpy(sph->gres.line1, " GRES       ", sph->gres.column_width);
  strncpy(sph->gres.line2, "(NODE-COUNT)", sph->gres.column_width);
  sph->features.visible = 0;
  sph->features.column_width = 12;
  strncpy(sph->features.line1, " FEATURES   ", sph->features.column_width);
  strncpy(sph->features.line2, "(NODE-COUNT)", sph->features.column_width);
}

/* Sets all columns as visible */
void sp_headers_set_parameter_L(sp_headers_t *sph) {
#ifdef __slurmdb_cluster_rec_t_defined
  sph->cluster_name.visible = 0;
  sph->partition_name.visible = SHOW_LOCAL;
#else
  sph->partition_name.visible = 1;
#endif
  sph->partition_status.visible = 1;
  sph->free_cpu.visible = 1;
  sph->total_cpu.visible = 1;
  sph->free_node.visible = 1;
  sph->total_node.visible = 1;
  sph->waiting_resource.visible = 1;
  sph->waiting_other.visible = 1;
  sph->my_running.visible = 1;
  sph->my_waiting_resource.visible = 1;
  sph->my_waiting_other.visible = 1;
  sph->my_total.visible = 1;
  sph->min_nodes.visible = 1;
  sph->max_nodes.visible = 1;
  sph->max_cpus_per_node.visible = 1;
  sph->max_mem_per_cpu.visible = 1;
  sph->def_mem_per_cpu.visible = 1;
  sph->djt_time.visible = 1;
  sph->mjt_time.visible = 1;
  sph->min_core.visible = 1;
  sph->min_mem_gb.visible = 1;
  sph->partition_qos.visible = 1;
  sph->gres.visible = 1;
  sph->features.visible = 0;
  sph->min_core.column_width = 8;
  sph->min_mem_gb.column_width = 10;
}

/* If column visible, it prints header to the line1 and line2 strings */
void sp_column_header_print(char *line1, char *line2,
                            sp_column_header_t *spcol) {
  char cresult[SPART_MAX_COLUMN_SIZE];
  if (spcol->visible) {
    snprintf(cresult, SPART_MAX_COLUMN_SIZE, "%*s", spcol->column_width,
             spcol->line1);
    strncat(line1, cresult, spcol->column_width);
    snprintf(cresult, SPART_MAX_COLUMN_SIZE, "%*s", spcol->column_width,
             spcol->line2);
    strncat(line2, cresult, spcol->column_width);
    strncat(line1, " ", SPART_INFO_STRING_SIZE);
    strncat(line2, " ", SPART_INFO_STRING_SIZE);
  }
}

/* Prints visible Headers */
void sp_headers_print(sp_headers_t *sph) {
  char line1[SPART_INFO_STRING_SIZE];
  char line2[SPART_INFO_STRING_SIZE];

  line1[0] = 0;
  line2[0] = 0;

  sp_column_header_print(line1, line2, &(sph->hspace));
#ifdef __slurmdb_cluster_rec_t_defined
  sp_column_header_print(line1, line2, &(sph->cluster_name));
#endif
  sp_column_header_print(line1, line2, &(sph->partition_name));
  sp_column_header_print(line1, line2, &(sph->partition_status));
  sp_column_header_print(line1, line2, &(sph->free_cpu));
  sp_column_header_print(line1, line2, &(sph->total_cpu));
  sp_column_header_print(line1, line2, &(sph->waiting_resource));
  sp_column_header_print(line1, line2, &(sph->waiting_other));
  sp_column_header_print(line1, line2, &(sph->free_node));
  sp_column_header_print(line1, line2, &(sph->total_node));
  if (!(sph->hspace.visible)) {
    strncat(line1, "|", SPART_INFO_STRING_SIZE);
    strncat(line2, "|", SPART_INFO_STRING_SIZE);
  }
  sp_column_header_print(line1, line2, &(sph->my_running));
  sp_column_header_print(line1, line2, &(sph->my_waiting_resource));
  sp_column_header_print(line1, line2, &(sph->my_waiting_other));
  sp_column_header_print(line1, line2, &(sph->my_total));
  if (!(sph->hspace.visible)) {
    strncat(line1, "| ", SPART_INFO_STRING_SIZE);
    strncat(line2, "| ", SPART_INFO_STRING_SIZE);
  }
  sp_column_header_print(line1, line2, &(sph->min_nodes));
  sp_column_header_print(line1, line2, &(sph->max_nodes));
  sp_column_header_print(line1, line2, &(sph->max_cpus_per_node));
  sp_column_header_print(line1, line2, &(sph->def_mem_per_cpu));
  sp_column_header_print(line1, line2, &(sph->max_mem_per_cpu));
  sp_column_header_print(line1, line2, &(sph->djt_time));
  sp_column_header_print(line1, line2, &(sph->mjt_time));
  sp_column_header_print(line1, line2, &(sph->min_core));
  sp_column_header_print(line1, line2, &(sph->min_mem_gb));
  sp_column_header_print(line1, line2, &(sph->partition_qos));
  sp_column_header_print(line1, line2, &(sph->gres));
  sp_column_header_print(line1, line2, &(sph->features));
  printf("%s\n%s\n", line1, line2);
}

/* Condensed printing for big numbers (k,m) */
void con_print(uint32_t num, uint16_t column_width) {
  char cresult[SPART_MAX_COLUMN_SIZE];
  uint16_t clong, cres;
  snprintf(cresult, SPART_MAX_COLUMN_SIZE, "%d", num);
  clong = strlen(cresult);
  if (clong > column_width)
    cres = clong - column_width;
  else
    cres = 0;
  switch (cres) {
    case 1:
    case 2:
      printf("%*d%s ", column_width - 1, (uint32_t)(num / 1000), "k");
      break;

    case 3:
      printf("%*.1f%s ", column_width - 1, (float)(num / 1000000.0f), "m");
      break;

    case 4:
    case 5:
      printf("%*d%s ", column_width - 1, (uint32_t)(num / 1000000), "m");
      break;

    case 6:
      printf("%*.1f%s ", column_width - 1, (float)(num / 1000000000.0f), "g");
      break;

    case 7:
    case 8:
      printf("%*d%s ", column_width - 1, (uint32_t)(num / 1000000000), "g");
      break;

    default:
      printf("%*d ", column_width, num);
  }
}

/* Date printing */
void date_print(uint32_t time_to_show, uint16_t column_width,
                int show_as_date) {
  uint16_t tday;
  uint16_t thour;
  uint16_t tminute;
  int len;
  char cresult[SPART_MAX_COLUMN_SIZE];
  char ctmp[SPART_MAX_COLUMN_SIZE];

  cresult[0] = '\0';

  tday = time_to_show / 1440;
  thour = (time_to_show - (tday * 1440)) / 60;
  tminute = time_to_show - (tday * 1440) - (thour * (uint16_t)60);

  if (show_as_date == 0) {
    if ((time_to_show == INFINITE) || (time_to_show == NO_VAL))
      printf("    -      ");
    else {
      if (tday != 0) {
        snprintf(cresult, SPART_MAX_COLUMN_SIZE, "%d days ", tday);
      }
      if (thour != 0) {
        snprintf(ctmp, SPART_MAX_COLUMN_SIZE, "%d hour ", thour);
        strncat(cresult, ctmp, column_width);
      }
      if (tminute != 0) {
        snprintf(ctmp, SPART_MAX_COLUMN_SIZE, "%d mins ", tminute);
        strncat(cresult, ctmp, column_width);
      }
      len = strlen(cresult);
      /* delete last space */
      cresult[len - 1] = '\0';
      if (len < column_width)
        printf("%10s ", cresult);
      else
        printf("%4d-%02d:%02d ", tday, thour, tminute);
    }
  } else {
    if ((time_to_show == INFINITE) || (time_to_show == NO_VAL))
      printf("    -      ");
    else {
      printf("%4d-%02d:%02d ", tday, thour, tminute);
    }
  }
}

/* Condensed printing for big numbers (k,m) to the string */
void con_strprint(char *str, uint16_t size, uint32_t num) {
  char cresult[SPART_MAX_COLUMN_SIZE];
  snprintf(cresult, SPART_MAX_COLUMN_SIZE, "%d", num);
  switch (strlen(cresult)) {
    case 5:
    case 6:
      snprintf(str, size, "%d%s", (uint32_t)(num / 1000), "k");
      break;

    case 7:
      snprintf(str, size, "%.1f%s", (float)(num / 1000000.0f), "m");
      break;

    case 8:
    case 9:
      snprintf(str, size, "%d%s", (uint32_t)(num / 1000000), "m");
      break;

    case 10:
      snprintf(str, size, "%.1f%s", (float)(num / 1000000000.0f), "g");
      break;

    case 11:
    case 12:
      snprintf(str, size, "%d%s", (uint32_t)(num / 1000000000), "g");
      break;

    default:
      snprintf(str, size, "%d", num);
  }
}

/* Prints a horizontal separetor such as ======= */
void seperator_print(const char ch, const int count) {
  int x;
  for (x = 0; x < count; x++) printf("%c", ch);
}

#ifdef SPART_SHOW_STATEMENT
void statement_print(const char *stfile, const char *stpartition,
                     const int total_width) {

  char re_str[SPART_INFO_STRING_SIZE];
  FILE *fo;
  int m, k;
  fo = fopen(stfile, "r");
  if (fo) {
    printf("\n");
    while (fgets(re_str, SPART_INFO_STRING_SIZE, fo)) {
      /* To correctly frame some wide chars, but not all */
      m = 0;
      for (k = 0; (re_str[k] != '\0') && k < SPART_INFO_STRING_SIZE; k++) {
        if ((re_str[k] < -58) && (re_str[k] > -62)) m++;
        if (re_str[k] == '\n') re_str[k] = '\0';
      }
      printf("  %s %-*s %s\n", SPART_STATEMENT_QUEUE_LINEPRE, 92 + m, re_str,
             SPART_STATEMENT_QUEUE_LINEPOST);
    }
    printf("  %s ", SPART_STATEMENT_QUEUE_LINEPRE);
    seperator_print('-', total_width);
    printf(" %s\n\n", SPART_STATEMENT_QUEUE_LINEPOST);
    pclose(fo);
  } else {
    printf("  %s ", SPART_STATEMENT_QUEUE_LINEPRE);
    seperator_print('-', total_width);
    printf(" %s\n", SPART_STATEMENT_QUEUE_LINEPOST);
  }
}
#endif

/* Prints a partition info */
void partition_print(spart_info_t *sp, sp_headers_t *sph, int show_max_mem,
                     int show_as_date, int total_width) {
  char mem_result[SPART_INFO_STRING_SIZE];
  if (sp->visible) {
    if (sph->hspace.visible)
      printf("%*s ", sph->hspace.column_width, "COMMON VALUES:");
#ifdef __slurmdb_cluster_rec_t_defined
    if (sph->cluster_name.visible)
      printf("%*s ", sph->cluster_name.column_width, sp->cluster_name);
#endif
    if (sph->partition_name.visible)
      printf("%*s ", sph->partition_name.column_width, sp->partition_name);
    if (sph->partition_status.visible)
      printf("%*s ", sph->partition_status.column_width, sp->partition_status);
    if (sph->free_cpu.visible)
      con_print(sp->free_cpu, sph->free_cpu.column_width);
    if (sph->total_cpu.visible)
      con_print(sp->total_cpu, sph->total_cpu.column_width);
    if (sph->waiting_resource.visible)
      con_print(sp->waiting_resource, sph->waiting_resource.column_width);
    if (sph->waiting_other.visible)
      con_print(sp->waiting_other, sph->waiting_other.column_width);
    if (sph->free_node.visible)
      con_print(sp->free_node, sph->free_node.column_width);
    if (sph->total_cpu.visible)
      con_print(sp->total_node, sph->total_cpu.column_width);
    if (!(sph->hspace.visible)) printf("|");
    if (sph->my_running.visible)
      con_print(sp->my_running, sph->my_running.column_width);
    if (sph->my_waiting_resource.visible)
      con_print(sp->my_waiting_resource, sph->my_waiting_resource.column_width);
    if (sph->my_waiting_other.visible)
      con_print(sp->my_waiting_other, sph->my_waiting_other.column_width);
    if (sph->my_total.visible)
      con_print(sp->my_total, sph->my_total.column_width);
    if (!(sph->hspace.visible)) printf("| ");
    if (sph->min_nodes.visible)
      con_print(sp->min_nodes, sph->min_nodes.column_width);
    if (sph->max_nodes.visible) {
      if (sp->max_nodes == UINT_MAX)
        printf("%*s ", sph->max_nodes.column_width, "-");
      else
        con_print(sp->max_nodes, sph->max_nodes.column_width);
    }
    if (sph->max_cpus_per_node.visible) {
      if ((sp->max_cpus_per_node == UINT_MAX) || (sp->max_cpus_per_node == 0))
        printf("%*s ", sph->max_cpus_per_node.column_width, "-");
      else
        con_print(sp->max_cpus_per_node, sph->max_cpus_per_node.column_width);
    }
    if (sph->def_mem_per_cpu.visible) {
      if ((sp->def_mem_per_cpu == UINT_MAX) || (sp->def_mem_per_cpu == 0))
        printf("%*s ", sph->def_mem_per_cpu.column_width, "-");
      else
        con_print(sp->def_mem_per_cpu, sph->def_mem_per_cpu.column_width);
    }
    if (sph->max_mem_per_cpu.visible) {
      if ((sp->max_mem_per_cpu == UINT_MAX) || (sp->max_mem_per_cpu == 0))
        printf("%*s ", sph->max_mem_per_cpu.column_width, "-");
      else
        con_print(sp->max_mem_per_cpu, sph->max_mem_per_cpu.column_width);
    }
    if (sph->djt_time.visible) {
      date_print(sp->djt_time, sph->djt_time.column_width, show_as_date);
    }
    if (sph->mjt_time.visible) {
      date_print(sp->mjt_time, sph->mjt_time.column_width, show_as_date);
    }
    if (sph->min_core.visible) {
      if ((show_max_mem == 1) && (sp->min_core != sp->max_core))
        snprintf(mem_result, SPART_INFO_STRING_SIZE, "%d-%d", sp->min_core,
                 sp->max_core);
      else
        snprintf(mem_result, SPART_INFO_STRING_SIZE, "%*d",
                 sph->min_core.column_width, sp->min_core);
      printf("%*s ", sph->min_core.column_width, mem_result);
    }
    if (sph->min_mem_gb.visible) {
      if ((show_max_mem == 1) && (sp->min_mem_gb != sp->max_mem_gb))
        snprintf(mem_result, SPART_INFO_STRING_SIZE, "%d-%d", sp->min_mem_gb,
                 sp->max_mem_gb);
      else
        snprintf(mem_result, SPART_INFO_STRING_SIZE, "%*d",
                 sph->min_mem_gb.column_width, sp->min_mem_gb);
      printf("%*s ", sph->min_mem_gb.column_width, mem_result);
    }
    if (sph->partition_qos.visible)
      printf("%*s ", sph->partition_qos.column_width, sp->partition_qos);

    if (sph->gres.visible) printf("%-*s ", sph->gres.column_width, sp->gres);
    if (sph->features.visible)
      printf("%-*s ", sph->features.column_width, sp->features);
    printf("\n");
#ifdef SPART_SHOW_STATEMENT
    if (sp->show_statement && !(sph->hspace.visible)) {
      snprintf(mem_result, SPART_INFO_STRING_SIZE, "%s%s%s%s",
               SPART_STATEMENT_DIR, SPART_STATEMENT_QUEPRE, sp->partition_name,
               SPART_STATEMENT_QUEPOST);
      statement_print(mem_result, sp->partition_name, total_width);
    }
#endif
  }
}

/* ========== MAIN ========== */
int main(int argc, char *argv[]) {
  uint32_t i, j;
  int k, m;
  int total_width = 0;

  /* Slurm defined types */
  slurm_ctl_conf_t *conf_info_msg_ptr = NULL;
  partition_info_msg_t *part_buffer_ptr = NULL;
  partition_info_t *part_ptr = NULL;
  node_info_msg_t *node_buffer_ptr = NULL;
  job_info_msg_t *job_buffer_ptr = NULL;

#if SLURM_VERSION_NUMBER > SLURM_VERSION_NUM(18, 7, 0) &&  \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 0) && \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 1)
  void **db_conn = NULL;
  slurmdb_assoc_cond_t assoc_cond;
  List assoc_list = NULL;
  ListIterator itr = NULL;
  slurmdb_assoc_rec_t *assoc;

  int user_acct_count = 0;
  char **user_acct = NULL;
#endif

  int user_qos_count = 0;
  char **user_qos = NULL;

  int user_group_count = SPART_MAX_GROUP_SIZE;
  char **user_group = NULL;

  uint32_t mem, cpus, min_mem, max_mem;
  uint32_t max_cpu, min_cpu, free_cpu, free_node;
  uint64_t max_mem_per_cpu = 0;
  uint64_t def_mem_per_cpu = 0;
  /* These values are default/unsetted values */
  const uint32_t default_min_nodes = 0, default_max_nodes = UINT_MAX;
  const uint64_t default_max_mem_per_cpu = 0;
  const uint64_t default_def_mem_per_cpu = 0;
  const uint32_t default_max_cpus_per_node = UINT_MAX;
  const uint32_t default_mjt_time = INFINITE;
  char *default_qos = "normal";

  uint16_t alloc_cpus = 0;
  char mem_result[SPART_INFO_STRING_SIZE];
  char strtmp[SPART_INFO_STRING_SIZE];
  char user_name[SPART_INFO_STRING_SIZE];
  int user_id;
#ifdef __slurmdb_cluster_rec_t_defined
  char cluster_name[SPART_INFO_STRING_SIZE];
#endif

#ifdef SPART_COMPILE_FOR_UHEM
  char *reason;
#endif

  uint32_t state;
  uint16_t tmp_lenght = 0;
  int show_max_mem = 0;
  int show_max_mem_per_cpu = 0;
  int show_max_cpus_per_node = 0;
  int show_def_mem_per_cpu = 0;
  uint16_t show_partition = 0;
  uint16_t show_partition_qos = 0;
  uint16_t show_min_nodes = 0;
  uint16_t show_max_nodes = 0;
  uint16_t show_mjt_time = 0;
  uint16_t show_djt_time = 0;
  int show_parameter_L = 0;
  int show_gres = 0;
  int show_features = 0;
  int show_all_partition = 0;
  int show_as_date = 0;
  int show_cluster_name = 0;
  int show_min_mem_gb = 0;
  int show_min_core = 0;
  int show_my_running = 1;
  int show_my_waiting_resource = 1;
  int show_my_waiting_other = 1;
  int show_my_total = 1;
  int show_simple = 0;

  uint16_t partname_lenght = 0;
#ifdef __slurmdb_cluster_rec_t_defined
  uint16_t clusname_lenght = 0;
#endif

  char partition_str[SPART_INFO_STRING_SIZE];
  char job_parts_str[SPART_INFO_STRING_SIZE];

  spart_info_t *spData = NULL;
  uint32_t partition_count = 0;

  uint16_t sp_gres_count = 0;
  sp_gres_info_t spgres[SPART_GRES_ARRAY_SIZE];

  uint16_t sp_features_count = 0;
  sp_gres_info_t spfeatures[SPART_GRES_ARRAY_SIZE];

  sp_headers_t spheaders;

  int show_info = 0;
  /* Get username */
  gid_t *groupIDs = NULL;
  struct passwd *pw;
  struct group *gr;

  pw = getpwuid(geteuid());
  strncpy(user_name, pw->pw_name, SPART_INFO_STRING_SIZE);
  user_id = pw->pw_uid;

  groupIDs = malloc(user_group_count * sizeof(gid_t));
  if (groupIDs == NULL) {
    slurm_perror("Can not allocate User group list");
    exit(1);
  }

  if (getgrouplist(user_name, pw->pw_gid, groupIDs, &user_group_count) == -1) {
    slurm_perror("Can not read User group list");
    exit(1);
  }

  user_group = malloc(user_group_count * sizeof(char *));
  for (k = 0; k < user_group_count; k++) {
    gr = getgrgid(groupIDs[k]);
    if (gr != NULL) {
      user_group[k] = malloc(SPART_INFO_STRING_SIZE * sizeof(char));
      strncpy(user_group[k], gr->gr_name, SPART_INFO_STRING_SIZE);
    }
  }

  free(groupIDs);

  /* Set default column visibility */
  sp_headers_set_defaults(&spheaders);

  for (k = 1; k < argc; k++) {
    if (strncmp(argv[k], "-m", 3) == 0) {
      show_max_mem = 1;
      spheaders.min_core.column_width = 8;
      spheaders.min_mem_gb.column_width = 10;
      continue;
    }

    if (strncmp(argv[k], "-a", 3) == 0) {
      show_partition |= SHOW_ALL;
      show_all_partition = 1;
      continue;
    }

#ifdef __slurmdb_cluster_rec_t_defined
    if (strncmp(argv[k], "-c", 3) == 0) {
      show_partition |= SHOW_FEDERATION;
      show_partition &= (~SHOW_LOCAL);
      spheaders.cluster_name.visible = 1;
      continue;
    }
#endif

    if (strncmp(argv[k], "-g", 3) == 0) {
      spheaders.gres.visible = 1;
      continue;
    }
    if (strncmp(argv[k], "-f", 3) == 0) {
      spheaders.features.visible = 1;
      continue;
    }

    if (strncmp(argv[k], "-i", 3) == 0) {
      show_info = 1;
      continue;
    }

    if (strncmp(argv[k], "-t", 3) == 0) {
      show_as_date = 1;
      continue;
    }

    if (strncmp(argv[k], "-s", 3) == 0) {
      show_gres = 0;
      show_min_nodes = 0;
      show_max_nodes = 0;
      show_max_cpus_per_node = 0;
      show_max_mem_per_cpu = 0;
      show_def_mem_per_cpu = 0;
      show_mjt_time = 0;
      show_djt_time = 0;
      show_partition_qos = 0;
      show_parameter_L = 0;
      show_simple = 1;
      spheaders.min_nodes.visible = 0;
      spheaders.max_nodes.visible = 0;
      spheaders.max_cpus_per_node.visible = 0;
      spheaders.max_mem_per_cpu.visible = 0;
      spheaders.def_mem_per_cpu.visible = 0;
      spheaders.djt_time.visible = 0;
      spheaders.mjt_time.visible = 0;
      spheaders.min_core.visible = 0;
      spheaders.min_mem_gb.visible = 0;
      spheaders.partition_qos.visible = 0;
      spheaders.gres.visible = 0;
      spheaders.features.visible = 0;
      continue;
    }

    if (strncmp(argv[k], "-J", 3) == 0) {
      show_my_running = 0;
      show_my_waiting_resource = 0;
      show_my_waiting_other = 0;
      show_my_total = 0;
      continue;
    }

    if (strncmp(argv[k], "-l", 3) == 0) {
      sp_headers_set_parameter_L(&spheaders);
      show_max_mem = 1;
#ifdef __slurmdb_cluster_rec_t_defined
      show_partition |= (SHOW_FEDERATION | SHOW_ALL);
#else
      show_partition |= SHOW_ALL;
#endif
      show_gres = 1;
      show_min_nodes = 1;
      show_max_nodes = 1;
      show_max_cpus_per_node = 1;
      show_max_mem_per_cpu = 1;
      show_def_mem_per_cpu = 1;
      show_mjt_time = 1;
      show_djt_time = 1;
      show_partition_qos = 1;
      show_parameter_L = 1;
      show_all_partition = 1;
      continue;
    }

    if (strncmp(argv[k], "-h", 3) != 0)
      printf("\nUnknown parameter: %s\n", argv[k]);
    spart_usage();
  }

  if (slurm_load_ctl_conf((time_t)NULL, &conf_info_msg_ptr)) {
    slurm_perror("slurm_load_ctl_conf error");
    exit(1);
  }

  if (slurm_load_jobs((time_t)NULL, &job_buffer_ptr, SHOW_ALL)) {
    slurm_perror("slurm_load_jobs error");
    exit(1);
  }

  if (slurm_load_node((time_t)NULL, &node_buffer_ptr, SHOW_ALL)) {
    slurm_perror("slurm_load_node error");
    exit(1);
  }

  if (slurm_load_partitions((time_t)NULL, &part_buffer_ptr, show_partition)) {
    slurm_perror("slurm_load_partitions error");
    exit(1);
  }

#ifdef __slurmdb_cluster_rec_t_defined
  strncpy(cluster_name, conf_info_msg_ptr->cluster_name, SPART_MAX_COLUMN_SIZE);
#endif

  /* to check that can we read pending jobs info */
  if (conf_info_msg_ptr->private_data != 0) {
    printf("WARNING: The Slurm settings have info restrictions!\n");

    /* to check that can we read pending jobs info */
    if (conf_info_msg_ptr->private_data & PRIVATE_DATA_JOBS) {
      printf("\tthe spart can not show other users' waiting jobs info!\n");
    }

    /* to check that can we read pending jobs info */
    if (conf_info_msg_ptr->private_data & PRIVATE_DATA_NODES) {
      printf("\tthe spart can not show node status info!\n");
    }

    /* to check that can we read pending jobs info */
    if (conf_info_msg_ptr->private_data & PRIVATE_DATA_PARTITIONS) {
      printf("\tthe spart can not show partition info!\n");
    }
    printf("\n");
  }

#if SLURM_VERSION_NUMBER > SLURM_VERSION_NUM(18, 7, 0) &&  \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 0) && \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 1)
  /* Getting user account info */
  db_conn = slurmdb_connection_get();
  if (errno != SLURM_SUCCESS) {
    slurm_perror("Can not connect to the slurm database");
    exit(1);
  }

  memset(&assoc_cond, 0, sizeof(slurmdb_assoc_cond_t));
  assoc_cond.user_list = slurm_list_create(NULL);
  slurm_list_append(assoc_cond.user_list, user_name);
  assoc_cond.acct_list = slurm_list_create(NULL);

  assoc_list = slurmdb_associations_get(db_conn, &assoc_cond);
  itr = slurm_list_iterator_create(assoc_list);

  user_acct_count = slurm_list_count(assoc_list);
  user_acct = malloc(user_acct_count * sizeof(char *));
  for (k = 0; k < user_acct_count; k++) {
    user_acct[k] = malloc(SPART_INFO_STRING_SIZE * sizeof(char));
    assoc = slurm_list_next(itr);
    strncpy(user_acct[k], assoc->acct, SPART_INFO_STRING_SIZE);
  }

  slurm_list_iterator_destroy(itr);
  slurm_list_destroy(assoc_list);
#endif

  char sh_str[SPART_INFO_STRING_SIZE];
  char re_str[SPART_INFO_STRING_SIZE];
  FILE *fo;
  char *p_str = NULL;
  char *t_str = NULL;

  if (show_info) {
    printf(" Your username: %s\n", user_name);
    printf(" Your group(s): ");
    for (k = 0; k < user_group_count; k++) {
      printf("%s ", user_group[k]);
    }
    printf("\n");
    printf(" Your account(s): ");
#if SLURM_VERSION_NUMBER > SLURM_VERSION_NUM(18, 7, 0) &&  \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 0) && \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 1)
    for (k = 0; k < user_acct_count; k++) {
      printf("%s ", user_acct[k]);
    }
#else
    /* run sacctmgr command to get associations from old slurm */
    snprintf(sh_str, SPART_INFO_STRING_SIZE,
             "sacctmgr list association format=account%%-30 where user=%s "
             "-n 2>/dev/null|tr -s '\n' ' '",
             user_name);
    fo = popen(sh_str, "r");
    if (fo) {
      fgets(re_str, SPART_INFO_STRING_SIZE, fo);
      printf("%s", re_str);
    }
    pclose(fo);

#endif
    printf("\n Your qos(s): ");
  }
  snprintf(sh_str, SPART_INFO_STRING_SIZE,
           "sacctmgr list association format=qos%%-30 where user=%s -n "
           "2>/dev/null |tr -s '\n, ' ' '",
           user_name);
  fo = popen(sh_str, "r");
  if (fo) {
    fgets(re_str, SPART_INFO_STRING_SIZE, fo);
    if (show_info) printf("%s", re_str);
    if (show_info) printf("\n\n");
    if (re_str[0] == '\0')
      user_qos_count = 1;
    else
      user_qos_count = 0;
    k = 0;
    for (j = 0; re_str[j]; j++)
      if (re_str[j] == ' ') user_qos_count++;

    user_qos = malloc(user_qos_count * sizeof(char *));
    for (p_str = strtok_r(re_str, " ", &t_str); p_str != NULL;
         p_str = strtok_r(NULL, " ", &t_str)) {
      user_qos[k] = malloc(SPART_INFO_STRING_SIZE * sizeof(char));
      strncpy(user_qos[k], p_str, SPART_INFO_STRING_SIZE);
      k++;
    }
  }
  pclose(fo);

  /* if (db_conn != NULL)
    slurmdb_connection_close(db_conn); */

  /* Initialize spart data for each partition */
  partition_count = part_buffer_ptr->record_count;
  spData = (spart_info_t *)malloc(partition_count * sizeof(spart_info_t));

  for (i = 0; i < partition_count; i++) {
    spData[i].free_cpu = 0;
    spData[i].total_cpu = 0;
    spData[i].free_node = 0;
    spData[i].total_node = 0;
    spData[i].waiting_resource = 0;
    spData[i].waiting_other = 0;
    spData[i].min_nodes = 0;
    spData[i].max_nodes = 0;
    spData[i].max_cpus_per_node = 0;
    spData[i].max_mem_per_cpu = 0;
    /* MaxJobTime */
    spData[i].mjt_time = 0;
    /* DefaultJobTime */
    spData[i].djt_time = 0;
#ifdef SPART_SHOW_STATEMENT
    spData[i].show_statement = show_info;
#endif
    spData[i].min_core = 0;
    spData[i].max_core = 0;
    spData[i].min_mem_gb = 0;
    spData[i].max_mem_gb = 0;
    spData[i].my_waiting_resource = 0;
    spData[i].my_waiting_other = 0;
    spData[i].my_running = 0;
    spData[i].my_total = 0;
    spData[i].visible = 1;
/* partition_name[] */
#ifdef __slurmdb_cluster_rec_t_defined
    spData[i].cluster_name[0] = 0;
#endif
    spData[i].partition_name[0] = 0;
    spData[i].partition_qos[0] = 0;
    spData[i].partition_status[0] = 0;
  }

  /* Finds resource/other waiting core count for each partition */
  for (i = 0; i < job_buffer_ptr->record_count; i++) {
    /* add ',' character at the begining and the end */
    strncpy(job_parts_str, ",", SPART_INFO_STRING_SIZE);
    strncat(job_parts_str, job_buffer_ptr->job_array[i].partition,
            SPART_INFO_STRING_SIZE);
    strncat(job_parts_str, ",", SPART_INFO_STRING_SIZE);

    for (j = 0; j < partition_count; j++) {
      /* add ',' character at the begining and the end */
      strncpy(partition_str, ",", SPART_INFO_STRING_SIZE);
      strncat(partition_str, part_buffer_ptr->partition_array[j].name,
              SPART_INFO_STRING_SIZE);
      strncat(partition_str + strlen(partition_str), ",",
              SPART_INFO_STRING_SIZE);

      if (strstr(job_parts_str, partition_str) != NULL) {
        if (job_buffer_ptr->job_array[i].job_state == JOB_PENDING) {
          if ((job_buffer_ptr->job_array[i].state_reason == WAIT_RESOURCES) ||
              (job_buffer_ptr->job_array[i].state_reason == WAIT_PRIORITY)) {
            spData[j].waiting_resource += job_buffer_ptr->job_array[i].num_cpus;
            if (job_buffer_ptr->job_array[i].user_id == user_id)
              spData[j].my_waiting_resource++;
          } else {
            spData[j].waiting_other += job_buffer_ptr->job_array[i].num_cpus;
            if (job_buffer_ptr->job_array[i].user_id == user_id)
              spData[j].my_waiting_other++;
          }
        } else {
          if ((job_buffer_ptr->job_array[i].user_id == user_id) &&
              (job_buffer_ptr->job_array[i].job_state == JOB_RUNNING))
            spData[j].my_running++;
        }
        if ((job_buffer_ptr->job_array[i].user_id == user_id) &&
            ((job_buffer_ptr->job_array[i].job_state == JOB_PENDING) ||
             (job_buffer_ptr->job_array[i].job_state == JOB_RUNNING) ||
             (job_buffer_ptr->job_array[i].job_state == JOB_SUSPENDED)))
          spData[j].my_total++;
      }
    }
  }

  show_gres = spheaders.gres.visible;
  show_features = spheaders.features.visible;
  for (i = 0; i < partition_count; i++) {
    part_ptr = &part_buffer_ptr->partition_array[i];

    min_mem = UINT_MAX;
    max_mem = 0;
    min_cpu = UINT_MAX;
    max_cpu = 0;
    free_cpu = 0;
    free_node = 0;
    alloc_cpus = 0;
    max_mem_per_cpu = 0;
    def_mem_per_cpu = 0;

    sp_gres_reset_counts(spgres, &sp_gres_count);
    sp_gres_reset_counts(spfeatures, &sp_features_count);

    for (j = 0; part_ptr->node_inx; j += 2) {
      if (part_ptr->node_inx[j] == -1) break;
      for (k = part_ptr->node_inx[j]; k <= part_ptr->node_inx[j + 1]; k++) {
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
        /* If gres will not show, don't work */
        if ((show_gres) && (node_buffer_ptr->node_array[k].gres != NULL)) {
          sp_gres_add(spgres, &sp_gres_count,
                      node_buffer_ptr->node_array[k].gres);
        }

        /* If features will not show, don't work */
        if (show_features) {
          if (node_buffer_ptr->node_array[k].features_act != NULL)
            sp_gres_add(spfeatures, &sp_features_count,
                        node_buffer_ptr->node_array[k].features_act);
          else if (node_buffer_ptr->node_array[k].features != NULL)
            sp_gres_add(spfeatures, &sp_features_count,
                        node_buffer_ptr->node_array[k].features);
        }
#ifdef SPART_COMPILE_FOR_UHEM
        reason = node_buffer_ptr->node_array[k].reason;
#endif

        /* The PowerSave_PwrOffState and PwrON_State_PowerSave control
         * for an alternative power saving solution we developed.
         * It required for showing power-off nodes as idle */
        if ((((state & NODE_STATE_DRAIN) != NODE_STATE_DRAIN) &&
             ((state & NODE_STATE_BASE) != NODE_STATE_DOWN) &&
             (state != NODE_STATE_UNKNOWN))
#ifdef SPART_COMPILE_FOR_UHEM
            ||
            (strncmp(reason, "PowerSave_PwrOffState", 21) == 0) ||
            (strncmp(reason, "PwrON_State_PowerSave", 21) == 0)
#endif
            ) {
          if (alloc_cpus == 0) free_node += 1;
          free_cpu += cpus - alloc_cpus;
        }
      }
    }

#ifdef __slurmdb_cluster_rec_t_defined
    if (part_ptr->cluster_name != NULL) {
      strncpy(spData[i].cluster_name, part_ptr->cluster_name,
              SPART_MAX_COLUMN_SIZE);
      tmp_lenght = strlen(part_ptr->cluster_name);
      if (tmp_lenght > clusname_lenght) clusname_lenght = tmp_lenght;
    } else {
      strncpy(spData[i].cluster_name, cluster_name, SPART_MAX_COLUMN_SIZE);
      tmp_lenght = strlen(cluster_name);
      if (tmp_lenght > clusname_lenght) clusname_lenght = tmp_lenght;
    }
#endif

    /* Partition States from more important to less important
     *  because, there is limited space. */
    if (part_ptr->flags & PART_FLAG_DEFAULT)
      strncat(spData[i].partition_status, "*", SPART_MAX_COLUMN_SIZE);
    if (part_ptr->flags & PART_FLAG_HIDDEN)
      strncat(spData[i].partition_status, ".", SPART_MAX_COLUMN_SIZE);

#if SLURM_VERSION_NUMBER > SLURM_VERSION_NUM(18, 7, 0) &&  \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 0) && \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 1)
    if ((part_ptr->allow_accounts != NULL) &&
        (strlen(part_ptr->allow_accounts) != 0)) {
      strncpy(strtmp, part_ptr->allow_accounts, SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_acct, user_acct_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the allow list */
        if (tmp_lenght != user_acct_count)
          strncat(spData[i].partition_status, "a", SPART_MAX_COLUMN_SIZE);
      } else {
        strncat(spData[i].partition_status, "A", SPART_MAX_COLUMN_SIZE);
        if (!show_all_partition) spData[i].visible = 0;
      }
    }

    if ((part_ptr->deny_accounts != NULL) &&
        (strlen(part_ptr->deny_accounts) != 0)) {
      strncpy(strtmp, part_ptr->deny_accounts, SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_acct, user_acct_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the deny list */
        if (tmp_lenght != user_acct_count)
          strncat(spData[i].partition_status, "a", SPART_MAX_COLUMN_SIZE);
        else {
          strncat(spData[i].partition_status, "A", SPART_MAX_COLUMN_SIZE);
          if (!show_all_partition) spData[i].visible = 0;
        }
      }
    }

    if ((part_ptr->allow_qos != NULL) && (strlen(part_ptr->allow_qos) != 0)) {
      strncpy(strtmp, part_ptr->allow_qos, SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_qos, user_qos_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the allow list */
        if (tmp_lenght != user_qos_count)
          strncat(spData[i].partition_status, "q", SPART_MAX_COLUMN_SIZE);
      } else {
        strncat(spData[i].partition_status, "Q", SPART_MAX_COLUMN_SIZE);
        if (!show_all_partition) spData[i].visible = 0;
      }
    }

    if ((part_ptr->deny_qos != NULL) && (strlen(part_ptr->deny_qos) != 0)) {
      strncpy(strtmp, part_ptr->deny_qos, SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_qos, user_qos_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the deny list */
        if (tmp_lenght != user_qos_count)
          strncat(spData[i].partition_status, "q", SPART_MAX_COLUMN_SIZE);
        else {
          strncat(spData[i].partition_status, "Q", SPART_MAX_COLUMN_SIZE);
          if (!show_all_partition) spData[i].visible = 0;
        }
      }
    }

    if ((part_ptr->allow_groups != NULL) &&
        (strlen(part_ptr->allow_groups) != 0)) {
      strncpy(strtmp, part_ptr->allow_groups, SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_group, user_group_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the allow list */
        if (tmp_lenght != user_group_count)
          strncat(spData[i].partition_status, "g", SPART_MAX_COLUMN_SIZE);
      } else {
        strncat(spData[i].partition_status, "G", SPART_MAX_COLUMN_SIZE);
        if (!show_all_partition) spData[i].visible = 0;
      }
    }

#endif

    if (strncmp(user_name, "root", SPART_INFO_STRING_SIZE) == 0) {
      if (part_ptr->flags & PART_FLAG_NO_ROOT) {
        strncat(spData[i].partition_status, "R", SPART_MAX_COLUMN_SIZE);
        if (!show_all_partition) spData[i].visible = 0;
      }
    } else {
      if (part_ptr->flags & PART_FLAG_ROOT_ONLY) {
        strncat(spData[i].partition_status, "R", SPART_MAX_COLUMN_SIZE);
        if (!show_all_partition) spData[i].visible = 0;
      }
    }

    if (!(part_ptr->state_up == PARTITION_UP)) {
      if (part_ptr->state_up == PARTITION_INACTIVE)
        strncat(spData[i].partition_status, "C", SPART_MAX_COLUMN_SIZE);
      if (part_ptr->state_up == PARTITION_DRAIN)
        strncat(spData[i].partition_status, "S", SPART_MAX_COLUMN_SIZE);
      if (part_ptr->state_up == PARTITION_DOWN)
        strncat(spData[i].partition_status, "D", SPART_MAX_COLUMN_SIZE);
    }

    if (part_ptr->flags & PART_FLAG_REQ_RESV)
      strncat(spData[i].partition_status, "r", SPART_MAX_COLUMN_SIZE);

    /* if (part_ptr->flags & PART_FLAG_EXCLUSIVE_USER)
      strncat(spData[i].partition_status, "x", SPART_MAX_COLUMN_SIZE);*/

    /* spgres (GRES) data converting to string */
    if (sp_gres_count == 0) {
      strncpy(spData[i].gres, "-", SPART_INFO_STRING_SIZE);
    } else {
      con_strprint(strtmp, SPART_INFO_STRING_SIZE, spgres[0].count);
      snprintf(mem_result, SPART_INFO_STRING_SIZE, "%s(%s)",
               spgres[0].gres_name, strtmp);
      strncpy(spData[i].gres, mem_result, SPART_INFO_STRING_SIZE);
      for (j = 1; j < sp_gres_count; j++) {
        con_strprint(strtmp, SPART_INFO_STRING_SIZE, spgres[j].count);
        snprintf(mem_result, SPART_INFO_STRING_SIZE, ",%s(%s)",
                 spgres[j].gres_name, strtmp);
        strncat(spData[i].gres, mem_result, SPART_INFO_STRING_SIZE);
      }
    }

    /* spfeatures data converting to string */
    if (sp_features_count == 0) {
      strncpy(spData[i].features, "-", SPART_INFO_STRING_SIZE);
    } else {
      con_strprint(strtmp, SPART_INFO_STRING_SIZE, spfeatures[0].count);
      snprintf(mem_result, SPART_INFO_STRING_SIZE, "%s(%s)",
               spfeatures[0].gres_name, strtmp);
      strncpy(spData[i].features, mem_result, SPART_INFO_STRING_SIZE);
      for (j = 1; j < sp_features_count; j++) {
        con_strprint(strtmp, SPART_INFO_STRING_SIZE, spfeatures[j].count);
        snprintf(mem_result, SPART_INFO_STRING_SIZE, ",%s(%s)",
                 spfeatures[j].gres_name, strtmp);
        strncat(spData[i].features, mem_result, SPART_INFO_STRING_SIZE);
      }
    }

    spData[i].free_cpu = free_cpu;
    spData[i].total_cpu = part_ptr->total_cpus;
    spData[i].free_node = free_node;
    spData[i].total_node = part_ptr->total_nodes;
    /* spData[i].waiting_resource and spData[i].waiting_other previously set
     */
    if (!show_simple) {
      spData[i].min_nodes = part_ptr->min_nodes;
      if ((part_ptr->min_nodes != default_min_nodes) && (spData[i].visible))
        show_min_nodes = 1;
      spData[i].max_nodes = part_ptr->max_nodes;
      if ((part_ptr->max_nodes != default_max_nodes) && (spData[i].visible))
        show_max_nodes = 1;
      spData[i].max_cpus_per_node = part_ptr->max_cpus_per_node;
      if ((part_ptr->max_cpus_per_node != default_max_cpus_per_node) &&
          (part_ptr->max_cpus_per_node != 0) && (spData[i].visible))
        show_max_cpus_per_node = 1;

      /* the def_mem_per_cpu and max_mem_per_cpu members contains
       * both FLAG bit (MEM_PER_CPU) for CPU/NODE selection, and values. */
      def_mem_per_cpu = part_ptr->def_mem_per_cpu;
      if (def_mem_per_cpu & MEM_PER_CPU) {
        strncpy(spheaders.def_mem_per_cpu.line2, "GB/CPU",
                spheaders.def_mem_per_cpu.column_width);
        def_mem_per_cpu = def_mem_per_cpu & (~MEM_PER_CPU);
      } else {
        strncpy(spheaders.def_mem_per_cpu.line2, "G/NODE",
                spheaders.def_mem_per_cpu.column_width);
      }
      spData[i].def_mem_per_cpu = (uint64_t)(def_mem_per_cpu / 1000u);
      if ((def_mem_per_cpu != default_def_mem_per_cpu) && (spData[i].visible))
        show_def_mem_per_cpu = 1;

      max_mem_per_cpu = part_ptr->max_mem_per_cpu;
      if (max_mem_per_cpu & MEM_PER_CPU) {
        strncpy(spheaders.max_mem_per_cpu.line2, "GB/CPU",
                spheaders.max_mem_per_cpu.column_width);
        max_mem_per_cpu = max_mem_per_cpu & (~MEM_PER_CPU);
      } else {
        strncpy(spheaders.max_mem_per_cpu.line2, "G/NODE",
                spheaders.max_mem_per_cpu.column_width);
      }
      spData[i].max_mem_per_cpu = (uint64_t)(max_mem_per_cpu / 1000u);
      if ((max_mem_per_cpu != default_max_mem_per_cpu) && (spData[i].visible))
        show_max_mem_per_cpu = 1;

      spData[i].mjt_time = part_ptr->max_time;
      if ((part_ptr->max_time != default_mjt_time) && (spData[i].visible))
        show_mjt_time = 1;
      spData[i].djt_time = part_ptr->default_time;
      if ((part_ptr->default_time != default_mjt_time) &&
          (part_ptr->default_time != NO_VAL) &&
          (part_ptr->default_time != part_ptr->max_time) && (spData[i].visible))
        show_djt_time = 1;
      spData[i].min_core = min_cpu;
      spData[i].max_core = max_cpu;
      spData[i].max_mem_gb = (uint16_t)(max_mem / 1000u);
      spData[i].min_mem_gb = (uint16_t)(min_mem / 1000u);

      if ((part_ptr->qos_char != NULL) && (strlen(part_ptr->qos_char) > 0)) {
        strncpy(spData[i].partition_qos, part_ptr->qos_char,
                SPART_MAX_COLUMN_SIZE);
        if (strncmp(part_ptr->qos_char, default_qos, SPART_MAX_COLUMN_SIZE) !=
            0)
          show_partition_qos = 1;
      } else
        strncpy(spData[i].partition_qos, "-", SPART_MAX_COLUMN_SIZE);
    }
    strncpy(spData[i].partition_name, part_ptr->name, SPART_MAX_COLUMN_SIZE);
    tmp_lenght = strlen(part_ptr->name);
    if (tmp_lenght > partname_lenght) partname_lenght = tmp_lenght;
  }

  if (partname_lenght > spheaders.partition_name.column_width) {
    m = partname_lenght - spheaders.partition_name.column_width;

    strncpy(strtmp, spheaders.partition_name.line1, SPART_INFO_STRING_SIZE);
    strcpy(spheaders.partition_name.line1, " ");
    for (k = 1; k < m; k++)
      strncat(spheaders.partition_name.line1, " ", partname_lenght);
    strncat(spheaders.partition_name.line1, strtmp, partname_lenght);

    strncpy(strtmp, spheaders.partition_name.line2, SPART_INFO_STRING_SIZE);
    strcpy(spheaders.partition_name.line2, " ");
    for (k = 1; k < m; k++)
      strncat(spheaders.partition_name.line2, " ", partname_lenght);
    strncat(spheaders.partition_name.line2, strtmp, partname_lenght);

    spheaders.partition_name.column_width = partname_lenght;
  }

#ifdef __slurmdb_cluster_rec_t_defined
  if (clusname_lenght > spheaders.cluster_name.column_width) {
    m = clusname_lenght - spheaders.cluster_name.column_width;

    strncpy(strtmp, spheaders.cluster_name.line1, SPART_INFO_STRING_SIZE);
    strcpy(spheaders.cluster_name.line1, " ");
    for (k = 1; k < m; k++)
      strncat(spheaders.cluster_name.line1, " ", clusname_lenght);
    strncat(spheaders.cluster_name.line1, strtmp, clusname_lenght);

    strncpy(strtmp, spheaders.cluster_name.line2, SPART_INFO_STRING_SIZE);
    strcpy(spheaders.cluster_name.line2, " ");
    for (k = 1; k < m; k++)
      strncat(spheaders.cluster_name.line2, " ", clusname_lenght);
    strncat(spheaders.cluster_name.line2, strtmp, clusname_lenght);

    spheaders.cluster_name.column_width = clusname_lenght;
  }
#endif

  /* If these column at default values, don't show */
  if ((!show_parameter_L) && (!show_simple)) {
    spheaders.min_nodes.visible = show_min_nodes;
    spheaders.max_nodes.visible = show_max_nodes;
    spheaders.max_cpus_per_node.visible = show_max_cpus_per_node;
    spheaders.max_mem_per_cpu.visible = show_max_mem_per_cpu;
    spheaders.def_mem_per_cpu.visible = show_def_mem_per_cpu;
    spheaders.mjt_time.visible = show_mjt_time;
    spheaders.djt_time.visible = show_djt_time;
    spheaders.partition_qos.visible = show_partition_qos;
  }
  spheaders.my_running.visible = show_my_running;
  spheaders.my_waiting_resource.visible = show_my_waiting_resource;
  spheaders.my_waiting_other.visible = show_my_waiting_other;
  spheaders.my_total.visible = show_my_total;

  if (show_all_partition) {
    for (i = 0; i < partition_count; i++) {
      spData[i].visible = 1;
    }
  }

  /* Output width calculation */
  total_width = 7; /* for || and space charecters */
  total_width += spheaders.partition_name.column_width;
  total_width += spheaders.partition_status.column_width;
  total_width += spheaders.free_cpu.column_width;
  total_width += spheaders.total_cpu.column_width;
  total_width += spheaders.free_node.column_width;
  total_width += spheaders.total_node.column_width;
  total_width += spheaders.waiting_resource.column_width;
  total_width += spheaders.waiting_other.column_width;

  /* Common Values scanning */
  /* reuse local show_xxx variables for different purpose */
  show_all_partition = 0; /* are there common feature */
  if (show_simple) {
    total_width += spheaders.my_running.column_width +
                   spheaders.my_waiting_resource.column_width +
                   spheaders.my_waiting_other.column_width +
                   spheaders.my_total.column_width + 4;
  } else {
    k = -1; /* first visible row (partition) */
    for (i = 0; i < partition_count; i++)
      if ((spData[i].visible) && (k == -1)) {
        k = i; /* first visible partition */
      }

    if (spheaders.my_running.visible) {/* is column visible */
      show_my_running = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].my_running != spData[k].my_running) {
            show_my_running = 0; /* it is not common */
            break;
          }
        }
      }
    }

    if (spheaders.my_waiting_resource.visible) {/* is column visible */
      show_my_waiting_resource = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].my_waiting_resource != spData[k].my_waiting_resource) {
            show_my_waiting_resource = 0; /* it is not common */
            break;
          }
        }
      }
    }

    if (spheaders.my_waiting_other.visible) {/* is column visible */
      show_my_waiting_other = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].my_waiting_other != spData[k].my_waiting_other) {
            show_my_waiting_other = 0; /* it is not common */
            break;
          }
        }
      }
    }

    if (spheaders.my_total.visible) {/* is column visible */
      show_my_total = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].my_total != spData[k].my_total) {
            show_my_total = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_my_waiting_resource && show_my_waiting_other &&
          show_my_running && show_my_total) {
        show_all_partition = 1;
        spheaders.my_running.visible = 0;
        spheaders.my_waiting_resource.visible = 0;
        spheaders.my_waiting_other.visible = 0;
        spheaders.my_total.visible = 0;
      } else
        total_width += spheaders.my_running.column_width +
                       spheaders.my_waiting_resource.column_width +
                       spheaders.my_waiting_other.column_width +
                       spheaders.my_total.column_width + 4;
    }

    if (spheaders.min_nodes.visible) {/* is column visible */
      show_min_nodes = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].min_nodes != spData[k].min_nodes) {
            show_min_nodes = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_min_nodes == 1) {
        show_all_partition = 1;
        spheaders.min_nodes.visible = 0;
      } else
        total_width += spheaders.min_nodes.column_width + 1;
    }

    if (spheaders.max_nodes.visible) {/* is column visible */
      show_max_nodes = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].max_nodes != spData[k].max_nodes) {
            show_max_nodes = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_max_nodes == 1) {
        show_all_partition = 1;
        spheaders.max_nodes.visible = 0;
      } else
        total_width += spheaders.max_nodes.column_width + 1;
    }

    if (spheaders.def_mem_per_cpu.visible) {/* is column visible */
      show_def_mem_per_cpu = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].def_mem_per_cpu != spData[k].def_mem_per_cpu) {
            show_def_mem_per_cpu = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_def_mem_per_cpu == 1) {
        show_all_partition = 1;
        spheaders.def_mem_per_cpu.visible = 0;
      } else
        total_width += spheaders.def_mem_per_cpu.column_width + 1;
    }

    if (spheaders.max_mem_per_cpu.visible) {/* is column visible */
      show_max_mem_per_cpu = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].max_mem_per_cpu != spData[k].max_mem_per_cpu) {
            show_max_mem_per_cpu = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_max_mem_per_cpu == 1) {
        show_all_partition = 1;
        spheaders.max_mem_per_cpu.visible = 0;
      } else
        total_width += spheaders.max_mem_per_cpu.column_width + 1;
    }

    if (spheaders.max_cpus_per_node.visible) {/* is column visible */
      show_max_cpus_per_node = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].max_cpus_per_node != spData[k].max_cpus_per_node) {
            show_max_cpus_per_node = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_max_cpus_per_node == 1) {
        show_all_partition = 1;
        spheaders.max_cpus_per_node.visible = 0;
      } else
        total_width += spheaders.max_cpus_per_node.column_width + 1;
    }

    if (spheaders.mjt_time.visible) {/* is column visible */
      show_mjt_time = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].mjt_time != spData[k].mjt_time) {
            show_mjt_time = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_mjt_time == 1) {
        show_all_partition = 1;
        spheaders.mjt_time.visible = 0;
      } else
        total_width += spheaders.mjt_time.column_width + 1;
    }

    if (spheaders.djt_time.visible) {/* is column visible */
      show_djt_time = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].djt_time != spData[k].djt_time) {
            show_djt_time = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_djt_time == 1) {
        show_all_partition = 1;
        spheaders.djt_time.visible = 0;
      } else
        total_width += spheaders.djt_time.column_width + 1;
    }

    if (spheaders.min_core.visible) {/* is column visible */
      show_min_core = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].min_core != spData[k].min_core) {
            show_min_core = 0; /* it is not common */
            break;
          }
        }
      }
      /* is max_core visible */
      if (show_max_mem) /* is column visible */
        for (i = 0; i < partition_count; i++) {
          if (spData[i].visible) /* is row visible */
          {
            if (spData[i].max_core != spData[k].max_core) {
              show_min_core = 0; /* it is not common */
              break;
            }
          }
        }
      if (show_min_core == 1) {
        show_all_partition = 1;
        spheaders.min_core.visible = 0;
      } else
        total_width += spheaders.min_core.column_width + 1;
    }

    if (spheaders.min_mem_gb.visible) {/* is column visible */
      show_min_mem_gb = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (spData[i].min_mem_gb != spData[k].min_mem_gb) {
            show_min_mem_gb = 0; /* it is not common */
            break;
          }
        }
      }
      /* is max_mem_gb visible */
      if (show_max_mem) /* is column visible */
        for (i = 0; i < partition_count; i++) {
          if (spData[i].visible) /* is row visible */
          {
            if (spData[i].max_mem_gb != spData[k].max_mem_gb) {
              show_min_core = 0; /* it is not common */
              break;
            }
          }
        }
      if (show_min_mem_gb == 1) {
        show_all_partition = 1;
        spheaders.min_mem_gb.visible = 0;
      } else
        total_width += spheaders.min_mem_gb.column_width + 1;
    }

    if (spheaders.partition_qos.visible) {/* is column visible */
      show_partition_qos = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (strncmp(spData[i].partition_qos, spData[k].partition_qos,
                      SPART_MAX_COLUMN_SIZE) != 0) {
            show_partition_qos = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_partition_qos == 1) {
        show_all_partition = 1;
        spheaders.partition_qos.visible = 0;
      } else
        total_width += spheaders.partition_qos.column_width + 1;
    }

    if (spheaders.gres.visible) {/* is column visible */
      show_gres = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (strncmp(spData[i].gres, spData[k].gres, SPART_MAX_COLUMN_SIZE) !=
              0) {
            show_gres = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_gres == 1) {
        show_all_partition = 1;
        spheaders.gres.visible = 0;
      } else
        total_width += spheaders.gres.column_width + 1;
    }

    if (spheaders.features.visible) {/* is column visible */
      show_features = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (strncmp(spData[i].features, spData[k].features,
                      SPART_MAX_COLUMN_SIZE) != 0) {
            show_features = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_features == 1) {
        show_all_partition = 1;
        spheaders.features.visible = 0;
      } else
        total_width += spheaders.features.column_width + 1;
    }

#ifdef __slurmdb_cluster_rec_t_defined
    if (spheaders.cluster_name.visible) {/* is column visible */
      show_cluster_name = 1;
      for (i = 0; i < partition_count; i++) {
        if (spData[i].visible) /* is row visible */
        {
          if (strncmp(spData[i].cluster_name, spData[k].cluster_name,
                      SPART_MAX_COLUMN_SIZE) != 0) {
            show_cluster_name = 0; /* it is not common */
            break;
          }
        }
      }
      if (show_cluster_name == 1) {
        show_all_partition = 1;
        spheaders.cluster_name.visible = 0;
      } else
        total_width += spheaders.cluster_name.column_width + 1;
    }
#endif
  }
  /* Headers is printing */
  sp_headers_print(&spheaders);

#ifdef SPART_SHOW_STATEMENT
  if (show_info) {
    printf("\n  %s ", SPART_STATEMENT_LINEPRE);
    seperator_print('=', total_width);
    printf(" %s\n\n", SPART_STATEMENT_LINEPOST);
  }
#endif

  /* Output is printing */
  for (i = 0; i < partition_count; i++) {
    partition_print(&(spData[i]), &spheaders, show_max_mem, show_as_date,
                    total_width);
  }

  /* Common values are printing */
  if (show_all_partition) {
    for (i = 0; i < partition_count; i++) {
      spData[i].visible = 0;
    }
    spData[k].visible = 1;

#ifdef __slurmdb_cluster_rec_t_defined
    spheaders.cluster_name.visible = 0;
#endif
    spheaders.partition_name.visible = 0;
    spheaders.partition_status.visible = 0;
    spheaders.free_cpu.visible = 0;
    spheaders.total_cpu.visible = 0;
    spheaders.free_node.visible = 0;
    spheaders.total_node.visible = 0;
    spheaders.waiting_resource.visible = 0;
    spheaders.waiting_other.visible = 0;
    spheaders.my_running.visible = 0;
    spheaders.my_waiting_resource.visible = 0;
    spheaders.my_waiting_other.visible = 0;
    spheaders.my_total.visible = 0;
    spheaders.min_nodes.visible = 0;
    spheaders.max_nodes.visible = 0;
    spheaders.max_cpus_per_node.visible = 0;
    spheaders.max_mem_per_cpu.visible = 0;
    spheaders.def_mem_per_cpu.visible = 0;
    spheaders.djt_time.visible = 0;
    spheaders.mjt_time.visible = 0;
    spheaders.min_core.visible = 0;
    spheaders.min_mem_gb.visible = 0;
    spheaders.partition_qos.visible = 0;
    spheaders.gres.visible = 0;
    spheaders.features.visible = 0;

    printf("\n");
#ifdef __slurmdb_cluster_rec_t_defined
    if (show_cluster_name) spheaders.cluster_name.visible = 1;
#endif
    if (show_my_running && show_my_waiting_resource && show_my_waiting_other &&
        show_my_total) {
      spheaders.my_running.visible = 1;
      spheaders.my_waiting_resource.visible = 1;
      spheaders.my_waiting_other.visible = 1;
      spheaders.my_total.visible = 1;
    }
    if (show_min_nodes) spheaders.min_nodes.visible = 1;
    if (show_max_nodes) spheaders.max_nodes.visible = 1;
    if (show_max_cpus_per_node) spheaders.max_cpus_per_node.visible = 1;
    if (show_def_mem_per_cpu) spheaders.def_mem_per_cpu.visible = 1;
    if (show_max_mem_per_cpu) spheaders.max_mem_per_cpu.visible = 1;
    if (show_djt_time) spheaders.djt_time.visible = 1;
    if (show_mjt_time) spheaders.mjt_time.visible = 1;
    if (show_min_core) spheaders.min_core.visible = 1;
    if (show_min_mem_gb) spheaders.min_mem_gb.visible = 1;
    if (show_partition_qos) spheaders.partition_qos.visible = 1;
    if (show_gres) spheaders.gres.visible = 1;
    if (show_features) spheaders.features.visible = 1;
    spheaders.hspace.visible = 1;
    sp_headers_print(&spheaders);
    partition_print(&(spData[k]), &spheaders, show_max_mem, show_as_date,
                    total_width);
  }

#ifdef SPART_SHOW_STATEMENT
  /* Statement is printing */
  fo = fopen(SPART_STATEMENT_DIR SPART_STATEMENT_FILE, "r");
  if (fo) {
    printf("\n  %s ", SPART_STATEMENT_LINEPRE);
    seperator_print('=', total_width);
    printf(" %s\n", SPART_STATEMENT_LINEPOST);
    while (fgets(re_str, SPART_INFO_STRING_SIZE, fo)) {
      /* To correctly frame some wide chars, but not all */
      m = 0;
      for (k = 0; (re_str[k] != '\0') && k < SPART_INFO_STRING_SIZE; k++) {
        if ((re_str[k] < -58) && (re_str[k] > -62)) m++;
        if (re_str[k] == '\n') re_str[k] = '\0';
      }
      printf("  %s %-*s %s\n", SPART_STATEMENT_LINEPRE, 92 + m, re_str,
             SPART_STATEMENT_LINEPOST);
    }
    printf("  %s ", SPART_STATEMENT_LINEPRE);
    seperator_print('=', total_width);
    printf(" %s\n", SPART_STATEMENT_LINEPOST);
    pclose(fo);
  }
#endif
#if SLURM_VERSION_NUMBER > SLURM_VERSION_NUM(18, 7, 0) &&  \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 0) && \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 1)
  /* free allocations */
  for (k = 0; k < user_acct_count; k++) {
    free(user_acct[k]);
  }
  free(user_acct);
#endif
  for (k = 0; k < user_group_count; k++) {
    free(user_group[k]);
  }
  free(user_group);

  free(spData);
  slurm_free_job_info_msg(job_buffer_ptr);
  slurm_free_node_info_msg(node_buffer_ptr);
  slurm_free_partition_info_msg(part_buffer_ptr);
  slurm_free_ctl_conf(conf_info_msg_ptr);
  exit(0);
}
