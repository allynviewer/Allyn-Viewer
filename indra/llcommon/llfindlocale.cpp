/** 
 * @file llfindlocale.cpp
 * @brief Detect system language setting
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#include "linden_common.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include "llwin32headers.h"
#include <winnt.h>
#endif
#include "llfindlocale.h"
static int
is_supported_ui_lang(const char *lang) {
  static const char *const langs[] = { "en", "es", "pt", "fr", "de", "it", "tr", "ru", "ja" };
  unsigned i;
  if (!lang) {
    return 0;
  }
  for (i = 0; i < LL_ARRAY_SIZE(langs); ++i) {
    if (0 == strcmp(lang, langs[i])) {
      return 1;
    }
  }
  return 0;
}
static int
is_lcchar(const int c) {
  return isalnum(c);
}
static void
lang_country_variant_from_envstring(const char *str,
                                    char **lang,
                                    char **country,
                                    char **variant) {
  int end = 0;
  int start;
  start = end;
  while (is_lcchar(str[end])) {
    ++end;
  }
  if (start != end) {
    int i;
    int len = end - start;
    char *s = (char*)malloc(len + 1);
    for (i=0; i<len; ++i) {
      s[i] = tolower(str[start + i]);
    }
    s[i] = '\0';
    *lang = s;
  } else {
    *lang = NULL;
  }
  if (str[end] && str[end]!=':') {
    ++end;
  }
  start = end;
  while (is_lcchar(str[end])) {
    ++end;
  }
  if (start != end) {
    int i;
    int len = end - start;
    char *s = (char*)malloc(len + 1);
    for (i=0; i<len; ++i) {
      s[i] = toupper(str[start + i]);
    }
    s[i] = '\0';
    *country = s;
  } else {
    *country = NULL;
  }
  if (str[end] && str[end]!=':') {
    ++end;
  }
  start = end;
  while (str[end] && str[end]!=':') {
    ++end;
  }
  if (start != end) {
    int i;
    int len = end - start;
    char *s = (char*)malloc(len + 1);
    for (i=0; i<len; ++i) {
      s[i] = str[start + i];
    }
    s[i] = '\0';
    *variant = s;
  } else {
    *variant = NULL;
  }
}
static int
accumulate_locstring(const char *str, FL_Locale *l) {
  char *lang = NULL;
  char *country = NULL;
  char *variant = NULL;
  if (str) {
    lang_country_variant_from_envstring(str, &lang, &country, &variant);
    if (lang) {
      l->lang = lang;
      l->country = country;
      l->variant = variant;
      return 1;
    }
  }
  free(lang); free(country); free(variant);
  return 0;
}
static int
accumulate_env(const char *name, FL_Locale *l) {
  char *env;
  char *lang = NULL;
  char *country = NULL;
  char *variant = NULL;
  env = getenv(name);
  if (env) {
    return accumulate_locstring(env, l);
  }
  free(lang); free(country); free(variant);
  return 0;
}
static void
canonise_fl(FL_Locale *l) {
  if (l->lang && 0 == strcmp(l->lang, "en")) {
    if (l->country && 0 == strcmp(l->country, "UK")) {
      free((void*)l->country);
      l->country = strdup("GB");
    }
  }
}
static void
reset_locale(FL_Locale *l) {
  if (l->lang) {
    free((void*)l->lang);
    l->lang = NULL;
  }
  if (l->country) {
    free((void*)l->country);
    l->country = NULL;
  }
  if (l->variant) {
    free((void*)l->variant);
    l->variant = NULL;
  }
}
#ifdef WIN32
#include <stdio.h>
#define ML(pn,sn) MAKELANGID(LANG_##pn, SUBLANG_##pn##_##sn)
#define MLN(pn) MAKELANGID(LANG_##pn, SUBLANG_DEFAULT)
struct IDToCode {
  LANGID id;
  const char*  code;
};
static const IDToCode both_to_code[] = {
  {ML(ENGLISH,US),           "en_US"},
  {ML(ENGLISH,CAN),          "en_CA"},
  {ML(ENGLISH,UK),           "en_GB"},
  {ML(ENGLISH,EIRE),         "en_IE"},
  {ML(ENGLISH,AUS),          "en_AU"},
  {MLN(GERMAN),              "de_DE"},
  {MLN(SPANISH),             "es_ES"},
  {ML(SPANISH,MEXICAN),      "es_MX"},
  {MLN(FRENCH),              "fr_FR"},
  {ML(FRENCH,CANADIAN),      "fr_CA"},
  {MLN(ITALIAN),             "it_IT"},
  {ML(PORTUGUESE,BRAZILIAN), "pt_BR"},
  {MLN(PORTUGUESE),          "pt_BR"},
  {MLN(TURKISH),             "tr_TR"},
  {MLN(RUSSIAN),             "ru_RU"},
  {MLN(JAPANESE),            "ja_JP"},
};
static const IDToCode primary_to_code[] = {
  {LANG_GERMAN,     "de"},
  {LANG_ENGLISH,    "en"},
  {LANG_SPANISH,    "es"},
  {LANG_FRENCH,     "fr"},
  {LANG_ITALIAN,    "it"},
  {LANG_PORTUGUESE, "pt"},
  {LANG_TURKISH,    "tr"},
  {LANG_RUSSIAN,    "ru"},
  {LANG_JAPANESE,   "ja"},
};
static int num_primary_to_code = LL_ARRAY_SIZE(primary_to_code);
static int num_both_to_code = LL_ARRAY_SIZE(both_to_code);
static const int
lcid_to_fl(LCID lcid,
           FL_Locale *rtn) {
  LANGID langid       = LANGIDFROMLCID(lcid);
  LANGID primary_lang = PRIMARYLANGID(langid);
  int i;
  for (i=0; i<num_both_to_code; ++i) {
    if (both_to_code[i].id == langid) {
      accumulate_locstring(both_to_code[i].code, rtn);
      return 1;
    }
  }
  for (i=0; i<num_primary_to_code; ++i) {
    if (primary_to_code[i].id == primary_lang) {
      accumulate_locstring(primary_to_code[i].code, rtn);
      return 1;
    }
  }
  return 0;
}
#endif
FL_Success
FL_FindLocale(FL_Locale **locale, FL_Domain domain) {
  FL_Success success = FL_FAILED;
  FL_Locale *rtn = (FL_Locale*)malloc(sizeof(FL_Locale));
  rtn->lang = NULL;
  rtn->country = NULL;
  rtn->variant = NULL;
#ifdef WIN32
  {
    LCID lcid = GetThreadLocale();
    if (lcid_to_fl(lcid, rtn)) {
      success = FL_CONFIDENT;
    }
    if (success == FL_FAILED) {
      if (accumulate_locstring("en_US", rtn)) {
        success = FL_DEFAULT_GUESS;
      }
    }
  }
#else
  {
    if (accumulate_env("LC_ALL", rtn) ||
        accumulate_env("LC_MESSAGES", rtn) ||
        accumulate_env("LANG", rtn) ||
        accumulate_env("LANGUAGE", rtn)) {
      success = FL_CONFIDENT;
    }
    if (success == FL_FAILED) {
      if (accumulate_locstring("en_US", rtn)) {
        success = FL_DEFAULT_GUESS;
      }
    }
  }
#endif
  if (success != FL_FAILED && !is_supported_ui_lang(rtn->lang)) {
    reset_locale(rtn);
    success = FL_FAILED;
    if (accumulate_locstring("en_US", rtn)) {
      success = FL_DEFAULT_GUESS;
    }
  }
  if (success != FL_FAILED) {
    canonise_fl(rtn);
  }
  *locale = rtn;
  return success;
}
void
FL_FreeLocale(FL_Locale **locale) {
  if (locale) {
    FL_Locale *l = *locale;
    if (l) {
      if (l->lang) {
        free((void*)l->lang);
      }
      if (l->country) {
        free((void*)l->country);
      }
      if (l->variant) {
        free((void*)l->variant);
      }
      free(l);
      *locale = NULL;
    }
  }
}
