/******************************************************************
 * spart    : a user-oriented partition info command for slurm
 * Author   : Cem Ahmet Mercan, 2019-02-16
 * Licence  : GNU General Public License v2.0
 * Note     : Some part of this code taken from slurm api man pages
 *******************************************************************/

#ifndef SPART_SPART_DATA_H_incl
#define SPART_SPART_DATA_H_incl

#include "spart_string.h"

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
      sp_strn2cpy(spga[*sp_gres_count].gres_name, SPART_INFO_STRING_SIZE,
                  grestok, SPART_INFO_STRING_SIZE);
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

/* it checks for permision string for user_spec list, return 0 if partition
 * should be hide */
int sp_check_permision_set_legend(char *permisions, char **user_spec,
                                  int user_spec_count, char *legendstr,
                                  const char *r_all, const char *r_some,
                                  const char *r_none) {
  int found_count;
  char strtmp[SPART_INFO_STRING_SIZE];
  if ((permisions != NULL) && (strlen(permisions) != 0)) {
    sp_strn2cpy(strtmp, SPART_INFO_STRING_SIZE, permisions,
                SPART_INFO_STRING_SIZE);
    found_count = sp_account_check(user_spec, user_spec_count, strtmp);
    if (found_count) {
      /* more than zero in the list */
      if (found_count != user_spec_count) {
        /* partial match */
        sp_strn2cat(legendstr, SPART_MAX_COLUMN_SIZE, r_some, 1);
      } else {
        /* found_count = ALL */
        if (r_all != NULL) {
          /* this is an deny list */
          sp_strn2cat(legendstr, SPART_MAX_COLUMN_SIZE, r_all, 1);
          return 0;
        }
      }
    } else {
      /* found_count = 0 */
      if (r_none != NULL) {
        /* this is an allow list */
        sp_strn2cat(legendstr, SPART_MAX_COLUMN_SIZE, r_none, 1);
        return 0;
      }
    }
  }
  return 1;
}

#endif /* SPART_SPART_DATA_H_incl */
