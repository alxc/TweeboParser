// Copyright (c) 2012-2013 Andre Martins
// All Rights Reserved.
//
// This file is part of TurboParser 2.1.
//
// TurboParser 2.1 is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TurboParser 2.1 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with TurboParser 2.1.  If not, see <http://www.gnu.org/licenses/>.

#include "DependencyInstanceNumeric.h"
#include <iostream>
#include <algorithm>

using namespace std;

void DependencyInstanceNumeric::Initialize(
    const DependencyDictionary &dictionary,
    DependencyInstance* instance) {
  TokenDictionary *token_dictionary = dictionary.GetTokenDictionary();
  int length = instance->size();
  int i;
  int id;

  int prefix_length = FLAGS_prefix_length;
  int suffix_length = FLAGS_suffix_length;
  bool form_case_sensitive = FLAGS_form_case_sensitive;

  Clear();

  // LPK: these lines are to create spaces for each field
  form_ids_.resize(length);
  brownall_ids_.resize(length);
  lemma_ids_.resize(length);
  prefix_ids_.resize(length);
  suffix_ids_.resize(length);

  // LPK: Note here, only the feats_ids is vector<vector<int>> while others are only vector<int>
  feats_ids_.resize(length);
  precompute_between_verbs_.resize(length+1);
  precompute_between_puncts_.resize(length+1);
  precompute_between_coords_.resize(length+1);
  for (i = 0; i < length+1; i++) {
    // Allocate the space for the precomputing vectors
    // The value of these will be filled later
    precompute_between_verbs_[i].resize(length+1);
    precompute_between_puncts_[i].resize(length+1);
    precompute_between_coords_[i].resize(length+1);
  }

  pos_ids_.resize(length);
  cpos_ids_.resize(length);
  brown4_ids_.resize(length);
  brown6_ids_.resize(length);
  shapes_.resize(length);
  is_noun_.resize(length);
  is_verb_.resize(length);
  is_punc_.resize(length);
  is_coord_.resize(length);
  heads_.resize(length);
  relations_.resize(length);
  selects_.resize(length);

  // LPK: TOKEN_UNKNOWN here seems to be the way to represent OOV (or OO POS...).
  for (i = 0; i < length; i++) {
    string form = instance->GetForm(i);
    if (!form_case_sensitive) {
      transform(form.begin(), form.end(), form.begin(), ::tolower);
    }
    id = token_dictionary->GetFormId(form);
    CHECK_LT(id, 0xffff);
    if (id < 0) id = TOKEN_UNKNOWN;
    form_ids_[i] = id;

    id = token_dictionary-> GetBrownAllId(instance->GetBrownAll(i));
    CHECK_LT(id, 0xffff);
    if (id < 0) id = TOKEN_UNKNOWN;
    brownall_ids_[i] = id;

    id = token_dictionary->GetLemmaId(instance->GetLemma(i));
    CHECK_LT(id, 0xffff);
    if (id < 0) id = TOKEN_UNKNOWN;
    lemma_ids_[i] = id;

    string prefix = form.substr(0, prefix_length);
    id = token_dictionary->GetPrefixId(prefix);
    CHECK_LT(id, 0xffff);
    if (id < 0) id = TOKEN_UNKNOWN;
    prefix_ids_[i] = id;

    int start = form.length() - suffix_length;
    if (start < 0) start = 0;
    string suffix = form.substr(start, suffix_length);
    id = token_dictionary->GetSuffixId(suffix);
    CHECK_LT(id, 0xffff);
    if (id < 0) id = TOKEN_UNKNOWN;
    suffix_ids_[i] = id;

    id = token_dictionary->GetPosTagId(instance->GetPosTag(i));
    CHECK_LT(id, 0xff);
    if (id < 0) id = TOKEN_UNKNOWN;
    pos_ids_[i] = id;

    id = token_dictionary->GetCoarsePosTagId(instance->GetCoarsePosTag(i));
    CHECK_LT(id, 0xff);
    if (id < 0) id = TOKEN_UNKNOWN;
    cpos_ids_[i] = id;

    id = token_dictionary->GetBrown4Id(instance->GetBrown4(i));
    CHECK_LT(id, 0xff);
    if (id < 0) id = TOKEN_UNKNOWN;
    brown4_ids_[i] = id;

    id = token_dictionary->GetBrown6Id(instance->GetBrown6(i));
    CHECK_LT(id, 0xff);
    if (id < 0) id = TOKEN_UNKNOWN;
    brown6_ids_[i] = id;

    feats_ids_[i].resize(instance->GetNumMorphFeatures(i));
    for (int j = 0; j < instance->GetNumMorphFeatures(i); ++j) {
      id = token_dictionary->GetMorphFeatureId(instance->GetMorphFeature(i, j));
      CHECK_LT(id, 0xffff);
      if (id < 0) id = TOKEN_UNKNOWN;
      feats_ids_[i][j] = id;
    }


    GetWordShape(instance->GetForm(i), &shapes_[i]);

    // Check whether the word is a noun, verb, punctuation or coordination.
    // Note: this depends on the POS tag string.
    // This procedure is taken from EGSTRA
    // (http://groups.csail.mit.edu/nlp/egstra/).
    
    is_noun_[i] = false;
    is_verb_[i] = false;
    is_punc_[i] = false;
    is_coord_[i] = false;

    const char* tag = instance->GetPosTag(i).c_str();
    if (tag[0] == 'v' || tag[0] == 'V') {
      is_verb_[i] = true;
    } else if (tag[0] == 'n' || tag[0] == 'N') {
      is_noun_[i] = true;
    } else if (strcmp(tag, "Punc") == 0 ||
               strcmp(tag, "$,") == 0 ||
               strcmp(tag, "$.") == 0 ||
               strcmp(tag, "PUNC") == 0 ||
               strcmp(tag, "punc") == 0 ||
               strcmp(tag, "F") == 0 ||
               strcmp(tag, "IK") == 0 ||
               strcmp(tag, "XP") == 0 ||
               strcmp(tag, ",") == 0 ||
               strcmp(tag, ";") == 0) {
      is_punc_[i] = true;
    } else if (strcmp(tag, "Conj") == 0 ||
               strcmp(tag, "KON") == 0 ||
               strcmp(tag, "conj") == 0 ||
               strcmp(tag, "Conjunction") == 0 ||
               strcmp(tag, "CC") == 0 ||
               strcmp(tag, "cc") == 0 ||
               strcmp(tag, "&") == 0) {
      is_coord_[i] = true;
    }

    heads_[i] = instance->GetHead(i);
    selects_[i] = instance->GetSelect(i);
    relations_[i] = dictionary.GetLabelAlphabet().Lookup(
        instance->GetDependencyRelation(i));
  }

  for (int left_position = 0; left_position < length; left_position++) {
    for (int right_position = left_position + 1; right_position < length; right_position++) {
      // Precompute the tables for between verbs, puncts and coords
      // Several flags.
      // 4 bits to denote the kind of flag.
      // Maximum will be 16 flags.
      uint8_t flag_between_verb = 0x0;
      uint8_t flag_between_punc = 0x1;
      uint8_t flag_between_coord = 0x2;

      int num_between_verb = 0;
      int num_between_punc = 0;
      int num_between_coord = 0;
      for (int i = left_position + 1; i < right_position; ++i) {
        if (IsVerb(i)) {
          ++num_between_verb;
        } else if (IsPunctuation(i)) {
          ++num_between_punc;
        } else if (IsCoordination(i)) {
          ++num_between_coord;
        }
      }

      // 4 bits to denote the number of occurrences for each flag.
      // Maximum will be 14 occurrences.
      // 15 will be used for extra uses.
      int max_occurrences = 14;
      if (num_between_verb > max_occurrences) num_between_verb = max_occurrences;
      if (num_between_punc > max_occurrences) num_between_punc = max_occurrences;
      if (num_between_coord > max_occurrences) num_between_coord = max_occurrences;
      flag_between_verb |= (num_between_verb << 4);
      flag_between_punc |= (num_between_punc << 4);
      flag_between_coord |= (num_between_coord << 4);

      // Write the results to the 2-dimension vectors for future use
      precompute_between_verbs_[left_position+1][right_position+1] = flag_between_verb;
      precompute_between_puncts_[left_position+1][right_position+1] = flag_between_punc;
      precompute_between_coords_[left_position+1][right_position+1] = flag_between_coord;

    }
  }
  for (int position = 0; position < length+1; position++) {
      // This is the special signal for the -1s.
      uint8_t flag_between_verb = 0x0;
      uint8_t flag_between_punc = 0x1;
      uint8_t flag_between_coord = 0x2;

      int max_occurrences = 15;
      flag_between_verb |= (max_occurrences << 4);
      flag_between_punc |= (max_occurrences << 4);
      flag_between_coord |= (max_occurrences << 4);

      precompute_between_verbs_[0][position] = flag_between_verb;
      precompute_between_puncts_[0][position] = flag_between_punc;
      precompute_between_coords_[0][position] = flag_between_coord;
  }
}
