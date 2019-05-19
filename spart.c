/******************************************************************
* spart    : a user-oriented partition info command for slurm
* Author   : Cem Ahmet Mercan, 2019-02-16
* Licence  : GNU General Public License v2.0
* Note     : Some part of this code taken from slurm api man pages
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pwd.h>
#include <slurm/slurm.h>
#include <slurm/slurmdb.h>
#include <slurm/slurm_errno.h>

/* #define SPART_COMPILE_FOR_UHEM */

#define SPART_INFO_STRING_SIZE 256
#define SPART_GRES_ARRAY_SIZE 64
#define SPART_MAX_COLUMN_SIZE 32


/* Prints Command Usage and Exit */
int spart_usage() {
  printf("\nUsage: spart [-m] [-a] "
#ifdef __slurmdb_cluster_rec_t_defined
"[-c] "
#endif
"[-g] [-l] [-h]\n\n");
  printf(
      "This program shows brief partition info with core count of available "
      "nodes and pending jobs.\n\n");
  printf(
      "In the STA-TUS column, the characters means, the partition is:\n"
      "\t*\tdefault partition (default queue),\n"
      "\t.\thidden partition,\n"
      "\tC\tclosed to both the job submit and run,\n"
      "\tS\tclosed to the job submit, but the submitted jobs will run,\n"
      "\tD\topen to the job submit, but the submitted jobs will not run,\n"
      "\tR\topen for only root, or closed to root (if you are root),\n"
      "\tA\tclosed to your account(s).\n\n");
  printf(
      "The RESOURCE PENDING column shows core counts of pending jobs "
      "because of the busy resource.\n\n");
  printf(
      "The OTHER PENDING column shows core counts of pending jobs because "
      "of the other reasons such\n as license or other limits.\n\n");
  printf(
      "If MIN NODES, MAX NODES, and MAXJOBTIME limits are not setted for the "
      "all partitions in your\n cluster, corresponding column(s) will not be "
      "shown.\n\n");
  printf(
      "The CORES PERNODE column shows the core count of the node with "
      "lowest core count in the partition.\n\n");
  printf(
      "The NODE MEM-GB column shows the memory of the lowest memory node "
      "in this partition.\n\n");
  printf("Parameters:\n\n");
  printf(
      "\t-m\tboth the lowest and highest values will be shown in the CORES"
      " PERNODE\n\t\tand NODE MEM-GB columns.\n\n");
  printf("\t-a\thidden partitions also be shown.\n\n");
#ifdef __slurmdb_cluster_rec_t_defined
  printf(
      "\t-c\tpartitions from federated clusters be shown.\n\n");
#endif
  printf(
      "\t-g\tthe ouput shows each GRES (gpu, mic etc.)"
      " defined in that partition\n\t\tand (in paranteses) the total number of "
      "nodes in that partition\n\t\tcontaining that GRES.\n\n");
  printf(
      "\t-l\tall posible columns will be shown, except"
      " the federated clusters column.\n\n");
  printf("\t-h\tshows this usage text.\n\n");
#ifdef SPART_COMPILE_FOR_UHEM
  printf("This is UHeM Version of the spart command.\n");
#endif
  printf("spart version 0.6.2\n\n");
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
  /* MaxJobTime */
  uint32_t mjt_time;
  uint16_t mjt_day;
  uint16_t mjt_hour;
  uint16_t mjt_minute;
  uint16_t min_core;
  uint16_t max_core;
  uint16_t min_mem_gb;
  uint16_t max_mem_gb;
  char partition_name[SPART_MAX_COLUMN_SIZE];
#ifdef __slurmdb_cluster_rec_t_defined
  char cluster_name[SPART_MAX_COLUMN_SIZE];
#endif
  char gres[SPART_INFO_STRING_SIZE];
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
  int found = 0;
  char *strtmp = NULL;
  char *grestok;

  for (grestok = strtok_r(partition_acct, ",", &strtmp); grestok != NULL;
       grestok = strtok_r(NULL, ",", &strtmp)) {

    for (i = 0; i < useract_count; i++) {
      if (strncmp(useracct[i], grestok, SPART_INFO_STRING_SIZE) == 0) {
        found = 1;
        break;
      }
    }
  }
  return found;
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
  sp_column_header_t min_nodes;
  sp_column_header_t max_nodes;
  /* MaxJobTime */
  sp_column_header_t mjt_time;
  sp_column_header_t min_core;
  sp_column_header_t min_mem_gb;
  sp_column_header_t gres;
} sp_headers_t;

/* Initialize all column headers */
void sp_headers_set_defaults(sp_headers_t *sph) {
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
  sph->min_nodes.visible = 1;
  sph->min_nodes.column_width = 6;
  strncpy(sph->min_nodes.line1, "   MIN", sph->min_nodes.column_width);
  strncpy(sph->min_nodes.line2, " NODES", sph->min_nodes.column_width);
  sph->max_nodes.visible = 1;
  sph->max_nodes.column_width = 6;
  strncpy(sph->max_nodes.line1, "   MAX", sph->max_nodes.column_width);
  strncpy(sph->max_nodes.line2, " NODES", sph->max_nodes.column_width);
  sph->mjt_time.visible = 1;
  sph->mjt_time.column_width = 10;
  strncpy(sph->mjt_time.line1, "MAXJOBTIME", sph->mjt_time.column_width);
  strncpy(sph->mjt_time.line2, " DAY-HR:MN", sph->mjt_time.column_width);
  sph->min_core.visible = 1;
  sph->min_core.column_width = 6;
  strncpy(sph->min_core.line1, " CORES", sph->min_core.column_width);
  strncpy(sph->min_core.line2, " /NODE", sph->min_core.column_width);
  sph->min_mem_gb.visible = 1;
  sph->min_mem_gb.column_width = 6;
  strncpy(sph->min_mem_gb.line1, "  NODE", sph->min_mem_gb.column_width);
  strncpy(sph->min_mem_gb.line2, "MEM-GB", sph->min_mem_gb.column_width);
  sph->gres.visible = 0;
  sph->gres.column_width = 8;
  strncpy(sph->gres.line1, "  GRES  ", sph->gres.column_width);
  strncpy(sph->gres.line2, "(COUNT) ", sph->gres.column_width);
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
  sph->min_nodes.visible = 1;
  sph->max_nodes.visible = 1;
  sph->mjt_time.visible = 1;
  sph->min_core.visible = 1;
  sph->min_mem_gb.visible = 1;
  sph->gres.visible = 1;
  sph->min_core.column_width = 8;
  sph->min_mem_gb.column_width = 10;
}

/* If column visible, it prints header to the line1 and line2 strings */
void sp_column_header_print(char *line1, char *line2,
                            sp_column_header_t *spcol) {
  char cresult[SPART_MAX_COLUMN_SIZE];
  if (spcol->visible) {
    sprintf(cresult, "%*s",spcol->column_width, spcol->line1);
    strncat(line1, cresult, spcol->column_width);
    sprintf(cresult, "%*s",spcol->column_width, spcol->line2);
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

#ifdef __slurmdb_cluster_rec_t_defined
  sp_column_header_print(line1, line2, &(sph->cluster_name));
#endif
  sp_column_header_print(line1, line2, &(sph->partition_name));
  sp_column_header_print(line1, line2, &(sph->partition_status));
  sp_column_header_print(line1, line2, &(sph->free_cpu));
  sp_column_header_print(line1, line2, &(sph->total_cpu));
  sp_column_header_print(line1, line2, &(sph->free_node));
  sp_column_header_print(line1, line2, &(sph->total_node));
  sp_column_header_print(line1, line2, &(sph->waiting_resource));
  sp_column_header_print(line1, line2, &(sph->waiting_other));
  sp_column_header_print(line1, line2, &(sph->min_nodes));
  sp_column_header_print(line1, line2, &(sph->max_nodes));
  sp_column_header_print(line1, line2, &(sph->mjt_time));
  sp_column_header_print(line1, line2, &(sph->min_core));
  sp_column_header_print(line1, line2, &(sph->min_mem_gb));
  sp_column_header_print(line1, line2, &(sph->gres));
  printf("%s\n%s\n", line1, line2);
}

/* Condensed printing for big numbers (k,m) */
void con_print(uint32_t num) {
  char cresult[64];
  sprintf(cresult, "%d", num);
  switch (strlen(cresult)) {
    case 5:
    case 6:
      printf("%5d%s ", (uint32_t)(num / 1000), "k");
      break;

    case 7:
      printf("%5.1f%s ", (float)(num / 1000000.0f), "m");
      break;

    case 8:
    case 9:
      printf("%5d%s ", (uint32_t)(num / 1000000), "m");
      break;

    case 10:
      printf("%5.1f%s ", (float)(num / 1000000000.0f), "g");
      break;

    case 11:
    case 12:
      printf("%5d%s ", (uint32_t)(num / 1000000000), "g");
      break;

    default:
      printf("%6d ", num);
  }
}

/* Condensed printing for big numbers (k,m) to the string */
void con_strprint(char *str, uint16_t size, uint32_t num) {
  char cresult[64];
  sprintf(cresult, "%d", num);
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

/* Prints a partition info */
void partition_print(spart_info_t *sp, sp_headers_t *sph, int show_max_mem) {
  char mem_result[SPART_INFO_STRING_SIZE];

#ifdef __slurmdb_cluster_rec_t_defined
  if (sph->cluster_name.visible) 
    printf("%*s ", sph->cluster_name.column_width, sp->cluster_name);
#endif
  if (sph->partition_name.visible) 
    printf("%*s ", sph->partition_name.column_width, sp->partition_name);
  if (sph->partition_status.visible) 
    printf("%*s ", sph->partition_status.column_width, sp->partition_status);
  if (sph->free_cpu.visible) con_print(sp->free_cpu);
  if (sph->total_cpu.visible) con_print(sp->total_cpu);
  if (sph->free_node.visible) con_print(sp->free_node);
  if (sph->total_cpu.visible) con_print(sp->total_node);
  if (sph->waiting_resource.visible) con_print(sp->waiting_resource);
  if (sph->waiting_other.visible) con_print(sp->waiting_other);
  if (sph->min_nodes.visible) con_print(sp->min_nodes);
  if (sph->max_nodes.visible) {
    if (sp->max_nodes == UINT_MAX)
      printf("     - ");
    else
      con_print(sp->max_nodes);
  }
  if (sph->mjt_time.visible) {
    if (sp->mjt_time == INFINITE)
      printf("    -     ");
    else
      printf("%4d-%02d:%02d", sp->mjt_day, sp->mjt_hour, sp->mjt_minute);
  }
  if (sph->min_core.visible) {
    if ((show_max_mem == 1) && (sp->min_core != sp->max_core))
      sprintf(mem_result, "%d-%d", sp->min_core, sp->max_core);
    else
      sprintf(mem_result, "%*d", sph->min_core.column_width, sp->min_core);
    printf(" %*s", sph->min_core.column_width, mem_result);
  }
  if (sph->min_mem_gb.visible) {
    if ((show_max_mem == 1) && (sp->min_mem_gb != sp->max_mem_gb))
      sprintf(mem_result, "%d-%d", sp->min_mem_gb, sp->max_mem_gb);
    else
      sprintf(mem_result, "%*d", sph->min_mem_gb.column_width, sp->min_mem_gb);
    printf(" %*s", sph->min_mem_gb.column_width, mem_result);
  }
  if (sph->gres.visible) printf(" %s", sp->gres);
  printf("\n");
}

/* ========== MAIN ========== */
int main(int argc, char *argv[]) {
  uint32_t i, j;
  int k;

  /* Slurm defined types */
  slurm_ctl_conf_t *conf_info_msg_ptr = NULL;
  partition_info_msg_t *part_buffer_ptr = NULL;
  partition_info_t *part_ptr = NULL;
  node_info_msg_t *node_buffer_ptr = NULL;
  job_info_msg_t *job_buffer_ptr = NULL;

  void **db_conn = NULL;
  slurmdb_assoc_cond_t assoc_cond;
  List assoc_list = NULL;
  ListIterator itr = NULL;
  slurmdb_assoc_rec_t *assoc;

  uint32_t mem, cpus, min_mem, max_mem;
  uint32_t max_cpu, min_cpu, free_cpu, free_node;
  /* These values are default/unsetted values */
  uint32_t default_min_nodes = 0, default_max_nodes = UINT_MAX;
  uint32_t default_mjt_time = INFINITE;

  uint16_t alloc_cpus = 0;
  char mem_result[SPART_INFO_STRING_SIZE];
  char strtmp[SPART_INFO_STRING_SIZE];
  char user_name[SPART_INFO_STRING_SIZE];
#ifdef __slurmdb_cluster_rec_t_defined
  char cluster_name[SPART_INFO_STRING_SIZE];
#endif

#ifdef SPART_COMPILE_FOR_UHEM
  char *reason;
#endif

  uint32_t state;
  int tmp_lenght = 0;
  int show_max_mem = 0;
  int show_partition = 0;
  int show_min_nodes = 0;
  int show_max_nodes = 0;
  int show_mjt_time = 0;
  int show_parameter_L = 0;
  int show_gres = 0;
  int partname_lenght = 0;
#ifdef __slurmdb_cluster_rec_t_defined
  int clusname_lenght = 0;
#endif

  char partition_str[SPART_INFO_STRING_SIZE];
  char job_parts_str[SPART_INFO_STRING_SIZE];

  int user_acct_count;
  char **user_acct = NULL;

  spart_info_t *spData = NULL;
  uint32_t partition_count = 0;

  uint16_t sp_gres_count = 0;
  sp_gres_info_t spgres[SPART_GRES_ARRAY_SIZE];

  sp_headers_t spheaders;

  /* Get username */
  struct passwd *pw = getpwuid(geteuid());
  strncpy(user_name, pw->pw_name, SPART_INFO_STRING_SIZE);

  /* Set default column visibility */
  sp_headers_set_defaults(&spheaders);

  for (i = 1; i < argc; i++) {
    if (strncmp(argv[i], "-m", 3) == 0) {
      show_max_mem = 1;
      spheaders.min_core.column_width = 8;
      spheaders.min_mem_gb.column_width = 10;
      continue;
    }

    if (strncmp(argv[i], "-a", 3) == 0) {
      show_partition |= SHOW_ALL;
      continue;
    }

#ifdef __slurmdb_cluster_rec_t_defined
    if (strncmp(argv[i], "-c", 3) == 0) {
      show_partition |= SHOW_FEDERATION;
      show_partition &= (~SHOW_LOCAL);
      spheaders.cluster_name.visible = 1;
      continue;
    }
#endif

    if (strncmp(argv[i], "-g", 3) == 0) {
      spheaders.gres.visible = 1;
      continue;
    }

    if (strncmp(argv[i], "-l", 3) == 0) {
      sp_headers_set_parameter_L(&spheaders);
      show_max_mem = 1;
#ifdef __slurmdb_cluster_rec_t_defined
      show_partition |= (SHOW_FEDERATION | SHOW_ALL | SHOW_LOCAL);
#else
      show_partition |=SHOW_ALL;
#endif
      show_gres = 1;
      show_min_nodes = 1;
      show_max_nodes = 1;
      show_mjt_time = 1;
      show_parameter_L = 1;
      continue;
    }

    if (strncmp(argv[i], "-h", 3) != 0)
      printf("\nUnknown parameter: %s\n", argv[i]);
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
  strncpy(cluster_name, conf_info_msg_ptr->cluster_name,
          SPART_MAX_COLUMN_SIZE);
#endif

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
  for (i = 0; i < user_acct_count; i++) {
    user_acct[i] = malloc(SPART_INFO_STRING_SIZE * sizeof(char));
    assoc = slurm_list_next(itr);
    strncpy(user_acct[i], assoc->acct, SPART_INFO_STRING_SIZE);
  }

  slurm_list_iterator_destroy(itr);
  slurm_list_destroy(assoc_list);

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
#ifdef __slurmdb_cluster_rec_t_defined
    spData[i].cluster_name[0] = 0;
#endif
    spData[i].partition_name[0] = 0;
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
              (job_buffer_ptr->job_array[i].state_reason == WAIT_PRIORITY))
            spData[j].waiting_resource += job_buffer_ptr->job_array[i].num_cpus;
          else
            spData[j].waiting_other += job_buffer_ptr->job_array[i].num_cpus;
        }
      }
    }
  }

  show_gres = spheaders.gres.visible;
  for (i = 0; i < partition_count; i++) {
    part_ptr = &part_buffer_ptr->partition_array[i];

    min_mem = UINT_MAX;
    max_mem = 0;
    min_cpu = UINT_MAX;
    max_cpu = 0;
    free_cpu = 0;
    free_node = 0;
    alloc_cpus = 0;

    sp_gres_reset_counts(spgres, &sp_gres_count);

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

#ifdef SPART_COMPILE_FOR_UHEM
        reason = node_buffer_ptr->node_array[k].reason;
#endif

        /* The PowerSave_PwrOffState and PwrON_State_PowerSave control
        * for an alternative power saving solution we developed.
        * It required for showing power-off nodes as idle */
        if ((((state & NODE_STATE_DRAIN) != NODE_STATE_DRAIN) &&
             ((state == NODE_STATE_DOWN) != NODE_STATE_DOWN))
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

    spData[i].free_cpu = free_cpu;
    spData[i].total_cpu = part_ptr->total_cpus;
    spData[i].free_node = free_node;
    spData[i].total_node = part_ptr->total_nodes;
    /* spData[i].waiting_resource and spData[i].waiting_other previously set */
    spData[i].min_nodes = part_ptr->min_nodes;
    if (part_ptr->min_nodes != default_min_nodes) show_min_nodes = 1;
    spData[i].max_nodes = part_ptr->max_nodes;
    if (part_ptr->max_nodes != default_max_nodes) show_max_nodes = 1;
    spData[i].mjt_time = part_ptr->max_time;
    if (part_ptr->max_time != default_mjt_time) show_mjt_time = 1;
    spData[i].mjt_day = part_ptr->max_time / 1440;
    spData[i].mjt_hour = (part_ptr->max_time - (spData[i].mjt_day * 1440)) / 60;
    spData[i].mjt_minute = part_ptr->max_time - (spData[i].mjt_day * 1440) -
                           (spData[i].mjt_hour * 60);
    spData[i].min_core = min_cpu;
    spData[i].max_core = max_cpu;
    spData[i].max_mem_gb = (uint16_t)(max_mem / 1000u);
    spData[i].min_mem_gb = (uint16_t)(min_mem / 1000u);
    strncpy(spData[i].partition_name, part_ptr->name, SPART_MAX_COLUMN_SIZE);
    tmp_lenght = strlen(part_ptr->name);
    if (tmp_lenght > partname_lenght) partname_lenght = tmp_lenght;

#ifdef __slurmdb_cluster_rec_t_defined
    if (part_ptr->cluster_name != NULL) {
      strncpy(spData[i].cluster_name, part_ptr->cluster_name,
              SPART_MAX_COLUMN_SIZE);
      tmp_lenght = strlen(part_ptr->cluster_name);
      if (tmp_lenght > partname_lenght) partname_lenght = tmp_lenght;
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

    if (part_ptr->allow_accounts != NULL) {
      strncpy(strtmp, part_ptr->allow_accounts, SPART_INFO_STRING_SIZE);
      if (!sp_account_check(user_acct, user_acct_count, strtmp))
        strncat(spData[i].partition_status, "A", SPART_MAX_COLUMN_SIZE);
    }

    if (part_ptr->deny_accounts != NULL) {
      strncpy(strtmp, part_ptr->deny_accounts, SPART_INFO_STRING_SIZE);
      if (sp_account_check(user_acct, user_acct_count, strtmp))
        strncat(spData[i].partition_status, "A", SPART_MAX_COLUMN_SIZE);
    }

    if (strncmp(user_name, "root", SPART_INFO_STRING_SIZE) == 0) {
      if (part_ptr->flags & PART_FLAG_NO_ROOT)
        strncat(spData[i].partition_status, "N", SPART_MAX_COLUMN_SIZE);
    } else {
      if (part_ptr->flags & PART_FLAG_ROOT_ONLY)
        strncat(spData[i].partition_status, "N", SPART_MAX_COLUMN_SIZE);
    }

    if (!(part_ptr->state_up & PARTITION_UP)) {
      if (part_ptr->state_up & PARTITION_INACTIVE)
        strncat(spData[i].partition_status, "C", SPART_MAX_COLUMN_SIZE);
      if (part_ptr->state_up & PARTITION_DRAIN)
        strncat(spData[i].partition_status, "S", SPART_MAX_COLUMN_SIZE);
      if (part_ptr->state_up & PARTITION_DOWN)
        strncat(spData[i].partition_status, "D", SPART_MAX_COLUMN_SIZE);
    }

    if (part_ptr->flags & PART_FLAG_REQ_RESV)
        strncat(spData[i].partition_status, "R", SPART_MAX_COLUMN_SIZE);

    /* spgre (GRES) data converting to string */
    if (sp_gres_count == 0) {
      strncpy(spData[i].gres, "-", SPART_INFO_STRING_SIZE);
    } else {
      con_strprint(strtmp, SPART_INFO_STRING_SIZE, spgres[0].count);
      sprintf(mem_result, "%s(%s)", spgres[0].gres_name, strtmp);
      strncpy(spData[i].gres, mem_result, SPART_INFO_STRING_SIZE);
      for (j = 1; j < sp_gres_count; j++) {
        con_strprint(strtmp, SPART_INFO_STRING_SIZE, spgres[j].count);
        sprintf(mem_result, ",%s(%s)", spgres[j].gres_name, strtmp);
        strncat(spData[i].gres, mem_result, SPART_INFO_STRING_SIZE);
      }
    }
  }

  if (partname_lenght > spheaders.partition_name.column_width) {
    j = partname_lenght - spheaders.partition_name.column_width;

    strncpy(strtmp, spheaders.partition_name.line1, SPART_INFO_STRING_SIZE);
    strcpy(spheaders.partition_name.line1, " ");
    for (k = 1; k < j; k++)
      strncat(spheaders.partition_name.line1, " ", partname_lenght);
    strncat(spheaders.partition_name.line1, strtmp, partname_lenght);

    strncpy(strtmp, spheaders.partition_name.line2, SPART_INFO_STRING_SIZE);
    strcpy(spheaders.partition_name.line2, " ");
    for (k = 1; k < j; k++)
      strncat(spheaders.partition_name.line2, " ", partname_lenght);
    strncat(spheaders.partition_name.line2, strtmp, partname_lenght);

    spheaders.partition_name.column_width = partname_lenght;
  }

#ifdef __slurmdb_cluster_rec_t_defined
  if (clusname_lenght > spheaders.cluster_name.column_width) {
    j = clusname_lenght - spheaders.cluster_name.column_width;

    strncpy(strtmp, spheaders.cluster_name.line1, SPART_INFO_STRING_SIZE);
    strcpy(spheaders.cluster_name.line1, " ");
    for (k = 1; k < j; k++)
      strncat(spheaders.cluster_name.line1, " ", clusname_lenght);
    strncat(spheaders.cluster_name.line1, strtmp, clusname_lenght);

    strncpy(strtmp, spheaders.cluster_name.line2, SPART_INFO_STRING_SIZE);
    strcpy(spheaders.cluster_name.line2, " ");
    for (k = 1; k < j; k++)
      strncat(spheaders.cluster_name.line2, " ", clusname_lenght);
    strncat(spheaders.cluster_name.line2, strtmp, clusname_lenght);

    spheaders.cluster_name.column_width = clusname_lenght;
  }
#endif

  /* If these column at default values, don't show */
  if (!show_parameter_L) {
    spheaders.min_nodes.visible = show_min_nodes;
    spheaders.max_nodes.visible = show_max_nodes;
    spheaders.mjt_time.visible = show_mjt_time;
  }

  /* Output is printing */
  sp_headers_print(&spheaders);

  for (i = 0; i < partition_count; i++) {
    partition_print(&(spData[i]), &spheaders, show_max_mem);
  }

  /* free allocations */
  free(spData);
  slurm_free_job_info_msg(job_buffer_ptr);
  slurm_free_node_info_msg(node_buffer_ptr);
  slurm_free_partition_info_msg(part_buffer_ptr);
  slurm_free_ctl_conf(conf_info_msg_ptr);
  exit(0);
}
