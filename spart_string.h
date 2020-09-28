/******************************************************************
 * spart    : a user-oriented partition info command for slurm
 * Author   : Cem Ahmet Mercan, 2019-02-16
 * Licence  : GNU General Public License v2.0
 * Note     : Some part of this code taken from slurm api man pages
 *******************************************************************/

#ifndef SPART_SPART_STRING_H_incl
#define SPART_SPART_STRING_H_incl

#include <string.h>

size_t sp_str_available(char *s, size_t maxlen) {
  size_t filled = strnlen(s, maxlen);
  if (filled < maxlen)
    return maxlen - filled - 1;
  else
    return 0;
}

char *sp_strn2cat(char *dest, size_t ndest, const char *src, size_t nsrc) {
  size_t available = 0;
  size_t filleddest = strnlen(dest, ndest);
  // size_t filledsrc = strnlen(src, nsrc);
  dest[ndest - 1] = 0;
  if (filleddest >= ndest) return dest;
  available = ndest - filleddest - 1;
  // if (filledsrc > nsrc) filledsrc = nsrc;
  // if (filledsrc > available) filledsrc = available;
  // return strncat(dest, src, filledsrc);
  if (available > nsrc) available = nsrc;
  return strncat(dest, src, available);
}

char *sp_strn2cpy(char *dest, size_t ndest, const char *src, size_t nsrc) {
  // size_t filledsrc = strnlen(src, nsrc);
  size_t available = nsrc;
  // printf("sp_strn2cpy dest=%s ndest=%d src=%s nsrc=%d\n",
  // dest,ndest,src,nsrc);
  // return strncpy(dest, src, nsrc);
  // if (filledsrc > nsrc) filledsrc = nsrc;
  // if (filledsrc > ndest) filledsrc = ndest;
  //  dest[ndest - 1] = 0;
  // return strncpy(dest, src, filledsrc);
  if (ndest < nsrc) available = ndest;
  return strncpy(dest, src, available);
}

/* Checks the user accounts present at the partition accounts */
/* Search each of the keys in a string which contains comma seperated keys */
int sp_account_check(char **key_list, int key_count, char *comma_sep_str) {
  uint16_t i;
  int *found = NULL;
  int parti = 0;
  char *strtmp = NULL;
  char *grestok;

  found = malloc(key_count * sizeof(int));
  for (i = 0; i < key_count; i++) found[i] = 0;

  for (grestok = strtok_r(comma_sep_str, ",", &strtmp); grestok != NULL;
       grestok = strtok_r(NULL, ",", &strtmp)) {
    for (i = 0; i < key_count; i++) {
      if (strncmp(key_list[i], grestok, SPART_INFO_STRING_SIZE) == 0) {
        found[i]++;
        break;
      }
    }
  }
  /* How many accounts/groups of user listed in the partition list */
  for (i = 0; i < key_count; i++)
    if (found[i] > 0) parti++;
  free(found);
  return parti;
}

/* Search for chr in str. If not fund, adds chr to str. */
void sp_char_check(char *str, int nstr, const char *chr, int nchr) {
  char c[2];
  int l = strnlen(chr, nchr);
  int i = 0;
  c[1] = 0;
  for (i = 0; i < l; i++) {
    c[0] = chr[i];
    if (strstr(str, c) == NULL) {
      sp_strn2cat(str, nstr, c, 1);
    }
  }
}

#endif /* SPART_SPART_STRING_H_incl */
