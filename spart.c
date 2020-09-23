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
#include "spart.h"
#include "spart_string.h"
#include "spart_data.h"
#include "spart_output.h"

/* ========== MAIN ========== */
int main(int argc, char *argv[]) {
  uint32_t i, j;
  int k, m, n;
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

  slurmdb_qos_cond_t qosn_cond;
  List qosn_list = NULL;
  ListIterator itr_qosn = NULL;

  slurmdb_qos_rec_t *qosn;
  slurmdb_assoc_rec_t *assoc;

  List qos_list = NULL;
  ListIterator itr_qos = NULL;
  int qosn_count = 0;

  char *qos = NULL;
  char *qos2 = NULL;
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

  sp_part_info_t *spData = NULL;
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
  sp_strn2cpy(user_name, SPART_INFO_STRING_SIZE, pw->pw_name,
              SPART_INFO_STRING_SIZE);
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
      sp_strn2cpy(user_group[k], SPART_INFO_STRING_SIZE, gr->gr_name,
                  SPART_INFO_STRING_SIZE);
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
    sp_spart_usage();
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
  sp_strn2cpy(cluster_name, SPART_MAX_COLUMN_SIZE,
              conf_info_msg_ptr->cluster_name, SPART_MAX_COLUMN_SIZE);
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
  user_qos_count = 0;

  for (k = 0; k < user_acct_count; k++) {
    assoc = slurm_list_next(itr);
    qos_list = assoc->qos_list;
    user_qos_count += slurm_list_count(qos_list);
  }

  // user_qos_count = user_acct_count*4;
  user_qos = malloc(user_qos_count * sizeof(char *));

  qosn_list = slurmdb_qos_get(db_conn, NULL);
  itr_qosn = slurm_list_iterator_create(qosn_list);
  qosn_count = slurm_list_count(qosn_list);

  // qosn = slurm_list_next(itr_qosn);
  // printf("QOS %s QOSid: %d\n", qosn->name, qosn->id);

  n = 0;
  slurm_list_iterator_reset(itr);
  for (k = 0; k < user_acct_count; k++) {
    user_acct[k] = malloc(SPART_INFO_STRING_SIZE * sizeof(char));
    assoc = slurm_list_next(itr);
    sp_strn2cpy(user_acct[k], SPART_INFO_STRING_SIZE, assoc->acct,
                SPART_INFO_STRING_SIZE);

    qos_list = assoc->qos_list;
    user_qos_count = slurm_list_count(qos_list);
    if (user_qos_count > 0) {
      itr_qos = slurm_list_iterator_create(qos_list);
      for (m = 0; m < user_qos_count; m++) {
        user_qos[n] = malloc(SPART_INFO_STRING_SIZE * sizeof(char));
        qos = slurm_list_next(itr_qos);
        qos2 = slurmdb_qos_str(qosn_list, atoi(qos));
        sp_strn2cpy(user_qos[n], SPART_INFO_STRING_SIZE, qos2,
                    SPART_INFO_STRING_SIZE);
        n++;
      }
    }
  }

  slurm_list_iterator_destroy(itr);
  slurm_list_iterator_destroy(itr_qos);
  // slurm_list_destroy(qos_list);
  slurm_list_destroy(assoc_list);
  slurm_list_iterator_destroy(itr_qosn);
  slurm_list_destroy(qosn_list);
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
#if SLURM_VERSION_NUMBER > SLURM_VERSION_NUM(18, 7, 0) &&  \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 0) && \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 1)
    printf(" Your account(s): ");
    for (k = 0; k < user_acct_count; k++) {
      printf("%s ", user_acct[k]);
    }
    printf("\n Your qos(s): ");
    for (k = 0; k < user_qos_count; k++) {
      printf("%s ", user_qos[k]);
    }
    printf("\n");
#else
    printf(" Your account(s): ");
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

    printf("\n Your qos(s): ");
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
        sp_strn2cpy(user_qos[k], SPART_INFO_STRING_SIZE, p_str,
                    SPART_INFO_STRING_SIZE);
        k++;
      }
    }
    pclose(fo);
#endif
  }

  /* if (db_conn != NULL)
    slurmdb_connection_close(db_conn); */

  /* Initialize spart data for each partition */
  partition_count = part_buffer_ptr->record_count;
  spData = (sp_part_info_t *)malloc(partition_count * sizeof(sp_part_info_t));

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
    sp_strn2cpy(job_parts_str, SPART_INFO_STRING_SIZE, ",",
                SPART_INFO_STRING_SIZE);
    sp_strn2cat(job_parts_str, SPART_INFO_STRING_SIZE,
                job_buffer_ptr->job_array[i].partition, SPART_INFO_STRING_SIZE);
    sp_strn2cat(job_parts_str, SPART_INFO_STRING_SIZE, ",", 1);

    for (j = 0; j < partition_count; j++) {
      /* add ',' character at the begining and the end */
      sp_strn2cpy(partition_str, SPART_INFO_STRING_SIZE, ",",
                  SPART_INFO_STRING_SIZE);
      sp_strn2cat(partition_str, SPART_INFO_STRING_SIZE,
                  part_buffer_ptr->partition_array[j].name,
                  SPART_INFO_STRING_SIZE);
      sp_strn2cat(partition_str + strlen(partition_str), SPART_INFO_STRING_SIZE,
                  ",", 1);

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
      sp_strn2cpy(spData[i].cluster_name, SPART_MAX_COLUMN_SIZE,
                  part_ptr->cluster_name, SPART_MAX_COLUMN_SIZE);
      tmp_lenght = strlen(part_ptr->cluster_name);
      if (tmp_lenght > clusname_lenght) clusname_lenght = tmp_lenght;
    } else {
      sp_strn2cpy(spData[i].cluster_name, SPART_MAX_COLUMN_SIZE, cluster_name,
                  SPART_MAX_COLUMN_SIZE);
      tmp_lenght = strlen(cluster_name);
      if (tmp_lenght > clusname_lenght) clusname_lenght = tmp_lenght;
    }
#endif

    /* Partition States from more important to less important
     *  because, there is limited space. */
    if (part_ptr->flags & PART_FLAG_DEFAULT)
      sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "*", 1);
    if (part_ptr->flags & PART_FLAG_HIDDEN)
      sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, ".", 1);

#if SLURM_VERSION_NUMBER > SLURM_VERSION_NUM(18, 7, 0) &&  \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 0) && \
    SLURM_VERSION_NUMBER != SLURM_VERSION_NUM(20, 2, 1)
    if ((part_ptr->allow_accounts != NULL) &&
        (strlen(part_ptr->allow_accounts) != 0)) {
      sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, part_ptr->allow_accounts,
                  SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_acct, user_acct_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the allow list */
        if (tmp_lenght != user_acct_count)
          sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "a",
                      1);
      } else {
        sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "A", 1);
        if (!show_all_partition) spData[i].visible = 0;
      }
    }

    if ((part_ptr->deny_accounts != NULL) &&
        (strlen(part_ptr->deny_accounts) != 0)) {
      sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, part_ptr->deny_accounts,
                  SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_acct, user_acct_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the deny list */
        if (tmp_lenght != user_acct_count)
          sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "a",
                      1);
        else {
          sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "A",
                      1);
          if (!show_all_partition) spData[i].visible = 0;
        }
      }
    }

    if ((part_ptr->allow_qos != NULL) && (strlen(part_ptr->allow_qos) != 0)) {
      sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, part_ptr->allow_qos,
                  SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_qos, user_qos_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the allow list */
        if (tmp_lenght != user_qos_count)
          sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "q",
                      1);
      } else {
        sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "Q", 1);
        if (!show_all_partition) spData[i].visible = 0;
      }
    }

    if ((part_ptr->deny_qos != NULL) && (strlen(part_ptr->deny_qos) != 0)) {
      sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, part_ptr->deny_qos,
                  SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_qos, user_qos_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the deny list */
        if (tmp_lenght != user_qos_count)
          sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "q",
                      1);
        else {
          sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "Q",
                      1);
          if (!show_all_partition) spData[i].visible = 0;
        }
      }
    }

    if ((part_ptr->allow_groups != NULL) &&
        (strlen(part_ptr->allow_groups) != 0)) {
      sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, part_ptr->allow_groups,
                  SPART_INFO_STRING_SIZE);
      tmp_lenght = sp_account_check(user_group, user_group_count, strtmp);
      if (tmp_lenght) {
        /* more then zero in the allow list */
        if (tmp_lenght != user_group_count)
          sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "g",
                      1);
      } else {
        sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "G", 1);
        if (!show_all_partition) spData[i].visible = 0;
      }
    }

#endif

    if (strncmp(user_name, "root", SPART_INFO_STRING_SIZE) == 0) {
      if (part_ptr->flags & PART_FLAG_NO_ROOT) {
        sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "R", 1);
        if (!show_all_partition) spData[i].visible = 0;
      }
    } else {
      if (part_ptr->flags & PART_FLAG_ROOT_ONLY) {
        sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "R", 1);
        if (!show_all_partition) spData[i].visible = 0;
      }
    }

    if (!(part_ptr->state_up == PARTITION_UP)) {
      if (part_ptr->state_up == PARTITION_INACTIVE)
        sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "C", 1);
      if (part_ptr->state_up == PARTITION_DRAIN)
        sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "S", 1);
      if (part_ptr->state_up == PARTITION_DOWN)
        sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "D", 1);
    }

    if (part_ptr->flags & PART_FLAG_REQ_RESV)
      sp_strn2cat(spData[i].partition_status, SPART_MAX_COLUMN_SIZE, "r", 1);

    /* if (part_ptr->flags & PART_FLAG_EXCLUSIVE_USER)
      strncat(spData[i].partition_status, "x", SPART_MAX_COLUMN_SIZE);*/

    /* spgres (GRES) data converting to string */
    if (sp_gres_count == 0) {
      sp_strn2cpy(spData[i].gres, SPART_INFO_STRING_SIZE, "-",
                  SPART_INFO_STRING_SIZE);
    } else {
      sp_con_strprint(strtmp, SPART_INFO_STRING_SIZE, spgres[0].count);
      snprintf(mem_result, SPART_INFO_STRING_SIZE, "%s(%s)",
               spgres[0].gres_name, strtmp);
      sp_strn2cpy(spData[i].gres, SPART_INFO_STRING_SIZE, mem_result,
                  SPART_INFO_STRING_SIZE);
      for (j = 1; j < sp_gres_count; j++) {
        sp_con_strprint(strtmp, SPART_INFO_STRING_SIZE, spgres[j].count);
        snprintf(mem_result, SPART_INFO_STRING_SIZE, ",%s(%s)",
                 spgres[j].gres_name, strtmp);
        sp_strn2cat(spData[i].gres, SPART_INFO_STRING_SIZE, mem_result,
                    SPART_INFO_STRING_SIZE);
      }
    }

    /* spfeatures data converting to string */
    if (sp_features_count == 0) {
      sp_strn2cpy(spData[i].features, SPART_INFO_STRING_SIZE, "-",
                  SPART_INFO_STRING_SIZE);
    } else {
      sp_con_strprint(strtmp, SPART_INFO_STRING_SIZE, spfeatures[0].count);
      snprintf(mem_result, SPART_INFO_STRING_SIZE, "%s(%s)",
               spfeatures[0].gres_name, strtmp);
      sp_strn2cpy(spData[i].features, SPART_INFO_STRING_SIZE, mem_result,
                  SPART_INFO_STRING_SIZE);
      for (j = 1; j < sp_features_count; j++) {
        sp_con_strprint(strtmp, SPART_INFO_STRING_SIZE, spfeatures[j].count);
        snprintf(mem_result, SPART_INFO_STRING_SIZE, ",%s(%s)",
                 spfeatures[j].gres_name, strtmp);
        sp_strn2cat(spData[i].features, SPART_INFO_STRING_SIZE, mem_result,
                    SPART_INFO_STRING_SIZE);
      }
    }

    spData[i].free_cpu = free_cpu;
    spData[i].total_cpu = part_ptr->total_cpus;
    spData[i].free_node = free_node;
    spData[i].total_node = part_ptr->total_nodes;

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
        sp_strn2cpy(spheaders.def_mem_per_cpu.line2,
                    spheaders.def_mem_per_cpu.column_width, "GB/CPU",
                    spheaders.def_mem_per_cpu.column_width);
        def_mem_per_cpu = def_mem_per_cpu & (~MEM_PER_CPU);
      } else {
        sp_strn2cpy(spheaders.def_mem_per_cpu.line2,
                    spheaders.def_mem_per_cpu.column_width, "G/NODE",
                    spheaders.def_mem_per_cpu.column_width);
      }
      spData[i].def_mem_per_cpu = (uint64_t)(def_mem_per_cpu / 1000u);
      if ((def_mem_per_cpu != default_def_mem_per_cpu) && (spData[i].visible))
        show_def_mem_per_cpu = 1;

      max_mem_per_cpu = part_ptr->max_mem_per_cpu;
      if (max_mem_per_cpu & MEM_PER_CPU) {
        sp_strn2cpy(spheaders.max_mem_per_cpu.line2,
                    spheaders.max_mem_per_cpu.column_width, "GB/CPU",
                    spheaders.max_mem_per_cpu.column_width);
        max_mem_per_cpu = max_mem_per_cpu & (~MEM_PER_CPU);
      } else {
        sp_strn2cpy(spheaders.max_mem_per_cpu.line2,
                    spheaders.max_mem_per_cpu.column_width, "G/NODE",
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
        sp_strn2cpy(spData[i].partition_qos, SPART_MAX_COLUMN_SIZE,
                    part_ptr->qos_char, SPART_MAX_COLUMN_SIZE);
        if (strncmp(part_ptr->qos_char, default_qos, SPART_MAX_COLUMN_SIZE) !=
            0)
          show_partition_qos = 1;
      } else
        sp_strn2cpy(spData[i].partition_qos, SPART_MAX_COLUMN_SIZE, "-",
                    SPART_MAX_COLUMN_SIZE);
    }
    sp_strn2cpy(spData[i].partition_name, SPART_MAX_COLUMN_SIZE, part_ptr->name,
                SPART_MAX_COLUMN_SIZE);
    tmp_lenght = strlen(part_ptr->name);
    if (tmp_lenght > partname_lenght) partname_lenght = tmp_lenght;
  }

  if (partname_lenght > spheaders.partition_name.column_width) {
    m = partname_lenght - spheaders.partition_name.column_width;

    sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, spheaders.partition_name.line1,
                SPART_INFO_STRING_SIZE);
    strcpy(spheaders.partition_name.line1, " ");
    for (k = 1; k < m; k++)
      sp_strn2cat(spheaders.partition_name.line1, partname_lenght, " ", 1);
    sp_strn2cat(spheaders.partition_name.line1, partname_lenght, strtmp,
                partname_lenght);

    sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, spheaders.partition_name.line2,
                SPART_INFO_STRING_SIZE);
    strcpy(spheaders.partition_name.line2, " ");
    for (k = 1; k < m; k++)
      sp_strn2cat(spheaders.partition_name.line2, partname_lenght, " ", 1);
    sp_strn2cat(spheaders.partition_name.line2, partname_lenght, strtmp,
                partname_lenght);

    spheaders.partition_name.column_width = partname_lenght;
  }

#ifdef __slurmdb_cluster_rec_t_defined
  if (clusname_lenght > spheaders.cluster_name.column_width) {
    m = clusname_lenght - spheaders.cluster_name.column_width;

    sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, spheaders.cluster_name.line1,
                SPART_INFO_STRING_SIZE);
    strcpy(spheaders.cluster_name.line1, " ");
    for (k = 1; k < m; k++)
      sp_strn2cat(spheaders.cluster_name.line1, clusname_lenght, " ", 1);
    sp_strn2cat(spheaders.cluster_name.line1, clusname_lenght, strtmp,
                clusname_lenght);

    sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, spheaders.cluster_name.line2,
                SPART_INFO_STRING_SIZE);
    strcpy(spheaders.cluster_name.line2, " ");
    for (k = 1; k < m; k++)
      sp_strn2cat(spheaders.cluster_name.line2, clusname_lenght, " ", 1);
    sp_strn2cat(spheaders.cluster_name.line2, clusname_lenght, strtmp,
                clusname_lenght);

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
    sp_seperator_print('=', total_width);
    printf(" %s\n\n", SPART_STATEMENT_LINEPOST);
  }
#endif

  /* Output is printing */
  for (i = 0; i < partition_count; i++) {
    sp_partition_print(&(spData[i]), &spheaders, show_max_mem, show_as_date,
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
    sp_partition_print(&(spData[k]), &spheaders, show_max_mem, show_as_date,
                       total_width);
  }

#ifdef SPART_SHOW_STATEMENT
  /* Statement is printing */
  fo = fopen(SPART_STATEMENT_DIR SPART_STATEMENT_FILE, "r");
  if (fo) {
    printf("\n  %s ", SPART_STATEMENT_LINEPRE);
    sp_seperator_print('=', total_width);
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
    sp_seperator_print('=', total_width);
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
