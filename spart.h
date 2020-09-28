/******************************************************************
 * spart    : a user-oriented partition info command for slurm
 * Author   : Cem Ahmet Mercan, 2019-02-16
 * Licence  : GNU General Public License v2.0
 * Note     : Some part of this code taken from slurm api man pages
 *******************************************************************/

#ifndef SPART_SPART_H_incl
#define SPART_SPART_H_incl

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

char *legend_info[] = {
    "* : default partition (default queue)",
    ". : hidden partition",
    "C : closed to both the job submit and run",
    "S : closed to the job submit, but the submitted jobs will run",
    "r : requires the reservation",
    "D : open to the job submit, but the submitted jobs will not run",
    "R : open for only root, or closed to root (if you are root)",
    "A : closed to all of your account(s)",
    "a : closed to some of your account(s)",
    "G : closed to all of your group(s)",
    "g : closed to some of your group(s)",
    "Q : closed to all of your QOS(s)",
    "q : closed to some of your QOS(s)"};
int legend_count = 12;

/* Prints Command Usage and Exit */
int sp_spart_usage() {
  printf(
      "\nUsage: spart [-m] [-a] "
#ifdef __slurmdb_cluster_rec_t_defined
      "[-c] "
#endif
      "[-g] [-i] [-t] [-f] [-l] [-s] [-J] [-v] [-h]\n\n");
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
  printf("\t-v\tshows info about STATUS LABELS.\n\n");
  printf("\t-h\tshows this usage text.\n\n");
#ifdef SPART_COMPILE_FOR_UHEM
  printf("This is UHeM Version of the spart command.\n");
#endif
  printf("spart version 1.4.0\n\n");
  exit(1);
}

/* To store partition info */
typedef struct sp_part_info {
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
} sp_part_info_t;

/* To storing info about a gres */
typedef struct sp_gres_info {
  uint32_t count;
  char gres_name[SPART_INFO_STRING_SIZE];
} sp_gres_info_t;

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

#endif /* SPART_SPART_H_incl */
