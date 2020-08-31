/******************************************************************
 * spart    : a user-oriented partition info command for slurm
 * Author   : Cem Ahmet Mercan, 2019-02-16
 * Licence  : GNU General Public License v2.0
 * Note     : Some part of this code taken from slurm api man pages
 *******************************************************************/

#ifndef SPART_SPART_OUTPUT_H_incl
#define SPART_SPART_OUTPUT_H_incl

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

/* Initialize all column headers */
void sp_headers_set_defaults(sp_headers_t *sph) {
  sph->hspace.visible = 0;
  sph->hspace.column_width = 17;
  sp_strn2cpy(sph->hspace.line1, sph->hspace.column_width, "           ",
              sph->hspace.column_width);
  sp_strn2cpy(sph->hspace.line2, sph->hspace.column_width, "           ",
              sph->hspace.column_width);
#ifdef __slurmdb_cluster_rec_t_defined
  sph->cluster_name.visible = 0;
  sph->cluster_name.column_width = 8;
  sp_strn2cpy(sph->cluster_name.line1, sph->cluster_name.column_width,
              " CLUSTER", sph->cluster_name.column_width);
  sp_strn2cpy(sph->cluster_name.line2, sph->cluster_name.column_width,
              "    NAME", sph->cluster_name.column_width);
#endif
  sph->partition_name.visible = 1;
  sph->partition_name.column_width = 10;
  sp_strn2cpy(sph->partition_name.line1, sph->partition_name.column_width,
              "     QUEUE", sph->partition_name.column_width);
  sp_strn2cpy(sph->partition_name.line2, sph->partition_name.column_width,
              " PARTITION", sph->partition_name.column_width);
  sph->partition_status.visible = 1;
  sph->partition_status.column_width = 3;
  sp_strn2cpy(sph->partition_status.line1, sph->partition_status.column_width,
              "STA", sph->partition_status.column_width);
  sp_strn2cpy(sph->partition_status.line2, sph->partition_status.column_width,
              "TUS", sph->partition_status.column_width);
  sph->free_cpu.visible = 1;
  sph->free_cpu.column_width = 6;
  sp_strn2cpy(sph->free_cpu.line1, sph->free_cpu.column_width, "  FREE",
              sph->free_cpu.column_width);
  sp_strn2cpy(sph->free_cpu.line2, sph->free_cpu.column_width, " CORES",
              sph->free_cpu.column_width);
  sph->total_cpu.visible = 1;
  sph->total_cpu.column_width = 6;
  sp_strn2cpy(sph->total_cpu.line1, sph->total_cpu.column_width, " TOTAL",
              sph->total_cpu.column_width);
  sp_strn2cpy(sph->total_cpu.line2, sph->total_cpu.column_width, " CORES",
              sph->total_cpu.column_width);
  sph->free_node.visible = 1;
  sph->free_node.column_width = 6;
  sp_strn2cpy(sph->free_node.line1, sph->free_node.column_width, "  FREE",
              sph->free_node.column_width);
  sp_strn2cpy(sph->free_node.line2, sph->free_node.column_width, " NODES",
              sph->free_node.column_width);
  sph->total_node.visible = 1;
  sph->total_node.column_width = 6;
  sp_strn2cpy(sph->total_node.line1, sph->total_node.column_width, " TOTAL",
              sph->total_node.column_width);
  sp_strn2cpy(sph->total_node.line2, sph->total_node.column_width, " NODES",
              sph->total_node.column_width);
  sph->waiting_resource.visible = 1;
  sph->waiting_resource.column_width = 6;
  sp_strn2cpy(sph->waiting_resource.line1, sph->waiting_resource.column_width,
              "RESORC", sph->waiting_resource.column_width);
  sp_strn2cpy(sph->waiting_resource.line2, sph->waiting_resource.column_width,
              "PENDNG", sph->waiting_resource.column_width);
  sph->waiting_other.visible = 1;
  sph->waiting_other.column_width = 6;
  sp_strn2cpy(sph->waiting_other.line1, sph->waiting_other.column_width,
              " OTHER", sph->waiting_other.column_width);
  sp_strn2cpy(sph->waiting_other.line2, sph->waiting_other.column_width,
              "PENDNG", sph->waiting_other.column_width);
  sph->my_running.visible = 1;
  sph->my_running.column_width = 4;
  sp_strn2cpy(sph->my_running.line1, sph->my_running.column_width, "YOUR",
              sph->my_running.column_width);
  sp_strn2cpy(sph->my_running.line2, sph->my_running.column_width, " RUN",
              sph->my_running.column_width);
  sph->my_waiting_resource.visible = 1;
  sph->my_waiting_resource.column_width = 4;
  sp_strn2cpy(sph->my_waiting_resource.line1,
              sph->my_waiting_resource.column_width, "PEND",
              sph->my_waiting_resource.column_width);
  sp_strn2cpy(sph->my_waiting_resource.line2,
              sph->my_waiting_resource.column_width, " RES",
              sph->my_waiting_resource.column_width);
  sph->my_waiting_other.visible = 1;
  sph->my_waiting_other.column_width = 4;
  sp_strn2cpy(sph->my_waiting_other.line1, sph->my_waiting_other.column_width,
              "PEND", sph->my_waiting_other.column_width);
  sp_strn2cpy(sph->my_waiting_other.line2, sph->my_waiting_other.column_width,
              "OTHR", sph->my_waiting_other.column_width);
  sph->my_total.visible = 1;
  sph->my_total.column_width = 4;
  sp_strn2cpy(sph->my_total.line1, sph->my_total.column_width, "YOUR",
              sph->my_total.column_width);
  sp_strn2cpy(sph->my_total.line2, sph->my_total.column_width, "TOTL",
              sph->my_total.column_width);
  sph->min_nodes.visible = 1;
  sph->min_nodes.column_width = 5;
  sp_strn2cpy(sph->min_nodes.line1, sph->min_nodes.column_width, "  MIN",
              sph->min_nodes.column_width);
  sp_strn2cpy(sph->min_nodes.line2, sph->min_nodes.column_width, "NODES",
              sph->min_nodes.column_width);
  sph->max_nodes.visible = 1;
  sph->max_nodes.column_width = 5;
  sp_strn2cpy(sph->max_nodes.line1, sph->max_nodes.column_width, "  MAX",
              sph->max_nodes.column_width);
  sp_strn2cpy(sph->max_nodes.line2, sph->max_nodes.column_width, "NODES",
              sph->max_nodes.column_width);
  sph->max_cpus_per_node.visible = 0;
  sph->max_cpus_per_node.column_width = 6;
  sp_strn2cpy(sph->max_cpus_per_node.line1, sph->max_cpus_per_node.column_width,
              "MAXCPU", sph->max_cpus_per_node.column_width);
  sp_strn2cpy(sph->max_cpus_per_node.line2, sph->max_cpus_per_node.column_width,
              " /NODE", sph->max_cpus_per_node.column_width);
  sph->max_mem_per_cpu.visible = 0;
  sph->max_mem_per_cpu.column_width = 6;
  sp_strn2cpy(sph->max_mem_per_cpu.line1, sph->max_mem_per_cpu.column_width,
              "MAXMEM", sph->max_mem_per_cpu.column_width);
  sp_strn2cpy(sph->max_mem_per_cpu.line2, sph->max_mem_per_cpu.column_width,
              "GB/CPU", sph->max_mem_per_cpu.column_width);
  sph->def_mem_per_cpu.visible = 0;
  sph->def_mem_per_cpu.column_width = 6;
  sp_strn2cpy(sph->def_mem_per_cpu.line1, sph->def_mem_per_cpu.column_width,
              "DEFMEM", sph->def_mem_per_cpu.column_width);
  sp_strn2cpy(sph->def_mem_per_cpu.line2, sph->def_mem_per_cpu.column_width,
              "GB/CPU", sph->def_mem_per_cpu.column_width);
  sph->djt_time.visible = 0;
  sph->djt_time.column_width = 10;
  sp_strn2cpy(sph->djt_time.line1, sph->djt_time.column_width, "   DEFAULT",
              sph->djt_time.column_width);
  sp_strn2cpy(sph->djt_time.line2, sph->djt_time.column_width, "  JOB-TIME",
              sph->djt_time.column_width);
  sph->mjt_time.visible = 1;
  sph->mjt_time.column_width = 10;
  sp_strn2cpy(sph->mjt_time.line1, sph->mjt_time.column_width, "   MAXIMUM",
              sph->mjt_time.column_width);
  sp_strn2cpy(sph->mjt_time.line2, sph->mjt_time.column_width, "  JOB-TIME",
              sph->mjt_time.column_width);
  sph->min_core.visible = 1;
  sph->min_core.column_width = 6;
  sp_strn2cpy(sph->min_core.line1, sph->min_core.column_width, " CORES",
              sph->min_core.column_width);
  sp_strn2cpy(sph->min_core.line2, sph->min_core.column_width, " /NODE",
              sph->min_core.column_width);
  sph->min_mem_gb.visible = 1;
  sph->min_mem_gb.column_width = 6;
  sp_strn2cpy(sph->min_mem_gb.line1, sph->min_mem_gb.column_width, "  NODE",
              sph->min_mem_gb.column_width);
  sp_strn2cpy(sph->min_mem_gb.line2, sph->min_mem_gb.column_width, "MEM-GB",
              sph->min_mem_gb.column_width);
  sph->partition_qos.visible = 1;
  sph->partition_qos.column_width = 6;
  sp_strn2cpy(sph->partition_qos.line1, sph->partition_qos.column_width,
              "   QOS", sph->partition_qos.column_width);
  sp_strn2cpy(sph->partition_qos.line2, sph->partition_qos.column_width,
              "  NAME", sph->partition_qos.column_width);
  sph->gres.visible = 0;
  sph->gres.column_width = 12;
  sp_strn2cpy(sph->gres.line1, sph->gres.column_width, " GRES       ",
              sph->gres.column_width);
  sp_strn2cpy(sph->gres.line2, sph->gres.column_width, "(NODE-COUNT)",
              sph->gres.column_width);
  sph->features.visible = 0;
  sph->features.column_width = 12;
  sp_strn2cpy(sph->features.line1, sph->features.column_width, " FEATURES   ",
              sph->features.column_width);
  sp_strn2cpy(sph->features.line2, sph->features.column_width, "(NODE-COUNT)",
              sph->features.column_width);
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
    sp_strn2cat(line1, SPART_INFO_STRING_SIZE, cresult, spcol->column_width);
    snprintf(cresult, SPART_MAX_COLUMN_SIZE, "%*s", spcol->column_width,
             spcol->line2);
    sp_strn2cat(line2, SPART_INFO_STRING_SIZE, cresult, spcol->column_width);
    sp_strn2cat(line1, SPART_INFO_STRING_SIZE, " ", 1);
    sp_strn2cat(line2, SPART_INFO_STRING_SIZE, " ", 1);
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
    sp_strn2cat(line1, SPART_INFO_STRING_SIZE, "|", 1);
    sp_strn2cat(line2, SPART_INFO_STRING_SIZE, "|", 1);
  }
  sp_column_header_print(line1, line2, &(sph->my_running));
  sp_column_header_print(line1, line2, &(sph->my_waiting_resource));
  sp_column_header_print(line1, line2, &(sph->my_waiting_other));
  sp_column_header_print(line1, line2, &(sph->my_total));
  if (!(sph->hspace.visible)) {
    sp_strn2cat(line1, SPART_INFO_STRING_SIZE, "| ", 2);
    sp_strn2cat(line2, SPART_INFO_STRING_SIZE, "| ", 2);
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
void sp_con_print(uint32_t num, uint16_t column_width) {
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
void sp_date_print(uint32_t time_to_show, uint16_t column_width,
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
        sp_strn2cat(cresult, column_width, ctmp, column_width);
      }
      if (tminute != 0) {
        snprintf(ctmp, SPART_MAX_COLUMN_SIZE, "%d mins ", tminute);
        sp_strn2cat(cresult, column_width, ctmp, column_width);
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
void sp_con_strprint(char *str, uint16_t size, uint32_t num) {
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
void sp_seperator_print(const char ch, const int count) {
  int x;
  for (x = 0; x < count; x++) printf("%c", ch);
}

#ifdef SPART_SHOW_STATEMENT
void sp_statement_print(const char *stfile, const char *stpartition,
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
    sp_seperator_print('-', total_width);
    printf(" %s\n\n", SPART_STATEMENT_QUEUE_LINEPOST);
    pclose(fo);
  } else {
    printf("  %s ", SPART_STATEMENT_QUEUE_LINEPRE);
    sp_seperator_print('-', total_width);
    printf(" %s\n", SPART_STATEMENT_QUEUE_LINEPOST);
  }
}
#endif

/* Prints a partition info */
void sp_partition_print(sp_part_info_t *sp, sp_headers_t *sph, int show_max_mem,
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
      sp_con_print(sp->free_cpu, sph->free_cpu.column_width);
    if (sph->total_cpu.visible)
      sp_con_print(sp->total_cpu, sph->total_cpu.column_width);
    if (sph->waiting_resource.visible)
      sp_con_print(sp->waiting_resource, sph->waiting_resource.column_width);
    if (sph->waiting_other.visible)
      sp_con_print(sp->waiting_other, sph->waiting_other.column_width);
    if (sph->free_node.visible)
      sp_con_print(sp->free_node, sph->free_node.column_width);
    if (sph->total_cpu.visible)
      sp_con_print(sp->total_node, sph->total_cpu.column_width);
    if (!(sph->hspace.visible)) printf("|");
    if (sph->my_running.visible)
      sp_con_print(sp->my_running, sph->my_running.column_width);
    if (sph->my_waiting_resource.visible)
      sp_con_print(sp->my_waiting_resource,
                   sph->my_waiting_resource.column_width);
    if (sph->my_waiting_other.visible)
      sp_con_print(sp->my_waiting_other, sph->my_waiting_other.column_width);
    if (sph->my_total.visible)
      sp_con_print(sp->my_total, sph->my_total.column_width);
    if (!(sph->hspace.visible)) printf("| ");
    if (sph->min_nodes.visible)
      sp_con_print(sp->min_nodes, sph->min_nodes.column_width);
    if (sph->max_nodes.visible) {
      if (sp->max_nodes == UINT_MAX)
        printf("%*s ", sph->max_nodes.column_width, "-");
      else
        sp_con_print(sp->max_nodes, sph->max_nodes.column_width);
    }
    if (sph->max_cpus_per_node.visible) {
      if ((sp->max_cpus_per_node == UINT_MAX) || (sp->max_cpus_per_node == 0))
        printf("%*s ", sph->max_cpus_per_node.column_width, "-");
      else
        sp_con_print(sp->max_cpus_per_node,
                     sph->max_cpus_per_node.column_width);
    }
    if (sph->def_mem_per_cpu.visible) {
      if ((sp->def_mem_per_cpu == UINT_MAX) || (sp->def_mem_per_cpu == 0))
        printf("%*s ", sph->def_mem_per_cpu.column_width, "-");
      else
        sp_con_print(sp->def_mem_per_cpu, sph->def_mem_per_cpu.column_width);
    }
    if (sph->max_mem_per_cpu.visible) {
      if ((sp->max_mem_per_cpu == UINT_MAX) || (sp->max_mem_per_cpu == 0))
        printf("%*s ", sph->max_mem_per_cpu.column_width, "-");
      else
        sp_con_print(sp->max_mem_per_cpu, sph->max_mem_per_cpu.column_width);
    }
    if (sph->djt_time.visible) {
      sp_date_print(sp->djt_time, sph->djt_time.column_width, show_as_date);
    }
    if (sph->mjt_time.visible) {
      sp_date_print(sp->mjt_time, sph->mjt_time.column_width, show_as_date);
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
      sp_statement_print(mem_result, sp->partition_name, total_width);
    }
#endif
  }
}

#endif /* SPART_SPART_OUTPUT_H_incl */
