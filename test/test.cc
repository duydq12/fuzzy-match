#include <gtest/gtest.h>

#include <fuzzy/fuzzy_match.hh>
#include <fuzzy/fuzzy_matcher_binarization.hh>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/lexical_cast.hpp>

namespace fs = boost::filesystem;

static fs::path data_dir;
static fs::path temp_dir;

static std::string get_data(const std::string& path) {
  return (data_dir / path).string();
}
static std::string get_temp(const std::string& path) {
  return (temp_dir / path).string();
}

TEST(FuzzyMatchTest, nofmi) {
  fuzzy::FuzzyMatch _fuzzyMatcher;
  ASSERT_THROW(fuzzy::import_binarized_fuzzy_matcher(get_data("non_existing.fmi"), _fuzzyMatcher),
               std::exception);
}

namespace boost {
    template<> 
    bool lexical_cast<bool, std::string>(const std::string& arg) {
        std::istringstream ss(arg);
        bool b;
        ss >> std::boolalpha >> b;
        return b;
    }
}

static void tests_matches(fuzzy::FuzzyMatch &fm, const std::string &testfile,
                          int min_subseq_length=2, float min_subseq_ratio=0) {
  std::ifstream ifs(get_data(testfile));
  std::string srcLine;
  while (getline(ifs, srcLine))
  {
    if (srcLine.length()==0 || srcLine[0] == '#')
      continue;
    std::vector<std::string> split_test;
    boost::split(split_test, srcLine, boost::is_any_of("\t"));
    ASSERT_GE((int)split_test.size(), 5);
    std::string test_id = split_test[0];
    std::string pattern = split_test[1];
    float fuzzy = boost::lexical_cast<float>(split_test[2]);
    bool non_perfect = boost::lexical_cast<bool>(split_test[3]);
    int nmatches = boost::lexical_cast<int>(split_test[4]);
    std::vector<fuzzy::FuzzyMatch::Match> matches;
    fm.match(pattern, fuzzy, nmatches, non_perfect, matches,
             min_subseq_length, min_subseq_ratio);
    /* check exact number of matches */
    size_t match_expected = (split_test.size()-5)/2;
    size_t match_found = matches.size();
    bool error = false;
    std::string error_desc = ">>>>> "+test_id+"("+pattern+";"+split_test[2]+";"+split_test[3]+";"+split_test[4]+")";
    for(size_t i=0; i<match_expected || i<match_found; i++) {
      error_desc += "\n";
      error_desc += "* expected=[";
      std::string possible_error;
      if (i<match_expected) {
        error_desc += split_test[5+2*i]+":"+split_test[5+2*i+1];
        possible_error = "=> MISSING MATCH";
      }
      error_desc += "]\tfound=[";
      if (i<match_found) {
        error_desc += boost::lexical_cast<std::string>(matches[i].score)+":"+matches[i].id;
        possible_error = "=> EXTRA MATCH";
      }
      error_desc += "]";
      std::string line_error;
      if (i<match_found && i<match_expected) {
        if (matches[i].id != split_test[5+2*i+1])
          line_error = "=> MISMATCH";
        else {
          float expected_score = boost::lexical_cast<float>(split_test[5+2*i]);
          float found_score = matches[i].score;
          if ((expected_score == 1 && found_score != 1) ||
              (expected_score != 1 && found_score == 1))
            line_error = "=> NON PERFECT";
          else if (std::abs(expected_score-found_score)>0.009)
            line_error = "=> INCORRECT SCORE";
        }
        if (!line_error.empty()) {
          error_desc += "\t"+line_error;
          error = true;
        }
      } else {
        error_desc += "\t"+possible_error;
        error = true;
      }
    }
    EXPECT_TRUE(!error) << error_desc;
  }  
}

TEST(FuzzyMatchTest, buildtm1_serialize) {
  fuzzy::FuzzyMatch _fuzzyMatcher(fuzzy::FuzzyMatch::pt_tag |
                                  fuzzy::FuzzyMatch::pt_nbr |
                                  fuzzy::FuzzyMatch::pt_cas);
  std::ifstream ifs(get_data("tm1"));
  std::string srcLine;
  int count=0;
  while (getline(ifs, srcLine))
  {
    /* index is the sentence */
    _fuzzyMatcher.add_tm(boost::lexical_cast<std::string>(++count)+"="+srcLine, srcLine);
  }
  _fuzzyMatcher.sort();
  tests_matches(_fuzzyMatcher, "test-tm1");
  fuzzy::export_binarized_fuzzy_matcher(get_temp("tm1.fmi"), _fuzzyMatcher);
  fuzzy::FuzzyMatch _fuzzyMatcher_reload;
  fuzzy::import_binarized_fuzzy_matcher(get_temp("tm1.fmi"), _fuzzyMatcher_reload);
  tests_matches(_fuzzyMatcher_reload, "test-tm1");
}

TEST(FuzzyMatchTest, buildtm1_pct) {
  fuzzy::FuzzyMatch _fuzzyMatcher(fuzzy::FuzzyMatch::pt_pct |
                                  fuzzy::FuzzyMatch::pt_nbr |
                                  fuzzy::FuzzyMatch::pt_cas);
  std::ifstream ifs(get_data("tm1"));
  std::string srcLine;
  int count=0;
  while (getline(ifs, srcLine))
  {
    testing::internal::CaptureStderr();
    /* index is the sentence */
    _fuzzyMatcher.add_tm(boost::lexical_cast<std::string>(++count)+"="+srcLine, srcLine);
    std::string warning = testing::internal::GetCapturedStderr();
    if (srcLine == ".")
      EXPECT_TRUE(warning.find("WARNING") != std::string::npos) << "warning: ["<<warning<<"]";
    else
      EXPECT_TRUE(warning == "") << "warning: ["<<warning<<"]";
  }
  _fuzzyMatcher.sort();
  tests_matches(_fuzzyMatcher, "test-tm1-pct");
}

TEST(FuzzyMatchTest, buildtm1_sep) {
  fuzzy::FuzzyMatch _fuzzyMatcher(fuzzy::FuzzyMatch::pt_tag |
                                  fuzzy::FuzzyMatch::pt_sep |
                                  fuzzy::FuzzyMatch::pt_cas |
                                  fuzzy::FuzzyMatch::pt_nbr);
  std::ifstream ifs(get_data("tm1"));
  std::string srcLine;
  int count=0;
  while (getline(ifs, srcLine))
  {
    /* index is the sentence */
    _fuzzyMatcher.add_tm(boost::lexical_cast<std::string>(++count)+"="+srcLine, srcLine);
  }
  _fuzzyMatcher.sort();
  tests_matches(_fuzzyMatcher, "test-tm1-sep");
}

TEST(FuzzyMatchTest, buildtm1_jnr) {
  fuzzy::FuzzyMatch _fuzzyMatcher(fuzzy::FuzzyMatch::pt_tag |
                                  fuzzy::FuzzyMatch::pt_jnr |
                                  fuzzy::FuzzyMatch::pt_cas |
                                  fuzzy::FuzzyMatch::pt_nbr);
  std::ifstream ifs(get_data("tm1"));
  std::string srcLine;
  int count=0;
  while (getline(ifs, srcLine))
  {
    /* index is the sentence */
    _fuzzyMatcher.add_tm(boost::lexical_cast<std::string>(++count)+"="+srcLine, srcLine);
  }
  _fuzzyMatcher.sort();
  tests_matches(_fuzzyMatcher, "test-tm1-jnr");
}

TEST(FuzzyMatchTest, buildtm1_nonbr_nocas) {
  fuzzy::FuzzyMatch _fuzzyMatcher(fuzzy::FuzzyMatch::pt_none);
  std::ifstream ifs(get_data("tm1"));
  std::string srcLine;
  int count=0;
  while (getline(ifs, srcLine))
  {
    /* index is the sentence */
    _fuzzyMatcher.add_tm(boost::lexical_cast<std::string>(++count)+"="+srcLine, srcLine);
  }
  _fuzzyMatcher.sort();
  tests_matches(_fuzzyMatcher, "test-tm1-nonbr");
}

TEST(FuzzyMatchTest, prebuild_tm1) {
  fuzzy::FuzzyMatch _fuzzyMatcher;
  fuzzy::import_binarized_fuzzy_matcher(get_data("tm1.fmi"), _fuzzyMatcher);
  tests_matches(_fuzzyMatcher, "test-tm1");  
}

TEST(FuzzyMatchTest, prebuild_tm1_oldapi) {
  fuzzy::FuzzyMatch _fuzzyMatcher;
  fuzzy::import_binarized_fuzzy_matcher(get_data("tm1.fmi"), _fuzzyMatcher);

  std::string sentence = "aa bb cc dd";
  std::vector<std::string> sentence_split;
  boost::split(sentence_split, sentence, boost::is_any_of(" "));
  float fuzzy = 0.6;
  int nmatches = 0;
  std::vector<fuzzy::FuzzyMatch::Match> matches;
  _fuzzyMatcher.match(sentence_split, fuzzy, nmatches, matches);
}

TEST(FuzzyMatchTest, prebuild_old_tm1) {
  fuzzy::FuzzyMatch _fuzzyMatcher;
  fuzzy::import_binarized_fuzzy_matcher(get_data("tm1.old.fmi"), _fuzzyMatcher);
  tests_matches(_fuzzyMatcher, "test-tm1");
}

TEST(FuzzyMatchTest, tm2) {
  fuzzy::FuzzyMatch _fuzzyMatcher;
  fuzzy::import_binarized_fuzzy_matcher(get_data("tm2.en.gz.fmi"), _fuzzyMatcher);
  tests_matches(_fuzzyMatcher, "test-tm2", 3, 0.3);
}

TEST(FuzzyMatchTest, small_sentence_matches) {
  fuzzy::FuzzyMatch _fuzzyMatcher;
  _fuzzyMatcher.add_tm("", "single");
  _fuzzyMatcher.add_tm("", "two words");
  _fuzzyMatcher.add_tm("", "three kind words");
  _fuzzyMatcher.sort();

  {
    std::string sentence = "single";
    std::vector<std::string> sentence_split;
    boost::split(sentence_split, sentence, boost::is_any_of(" "));
    std::vector<fuzzy::FuzzyMatch::Match> matches;
    int min_subseq_length = 3; // min_subseq_length > pattern length
    _fuzzyMatcher.match(sentence_split, 1, 1, matches, min_subseq_length);
    EXPECT_EQ(matches.size(), 1);
    EXPECT_EQ(matches[0].s_id, 0);
  }

  {
    std::string sentence = "two words";
    std::vector<std::string> sentence_split;
    boost::split(sentence_split, sentence, boost::is_any_of(" "));
    std::vector<fuzzy::FuzzyMatch::Match> matches;
    int min_subseq_length = 3; // min_subseq_length > pattern length
    _fuzzyMatcher.match(sentence_split, 1, 1, matches, min_subseq_length);
    EXPECT_EQ(matches.size(), 1);
    EXPECT_EQ(matches[0].s_id, 1);
  }

  {
    std::string sentence = "three kind words";
    std::vector<std::string> sentence_split;
    boost::split(sentence_split, sentence, boost::is_any_of(" "));
    std::vector<fuzzy::FuzzyMatch::Match> matches;
    int min_subseq_length = 3; // min_subseq_length == pattern length
    _fuzzyMatcher.match(sentence_split, 1, 1, matches, min_subseq_length);
    EXPECT_EQ(matches.size(), 1);
    EXPECT_EQ(matches[0].s_id, 2);
  }
}

TEST(FuzzyMatchTest, empty_token) {
  fuzzy::FuzzyMatch _fuzzyMatcher;
  _fuzzyMatcher.add_tm("", {"NMT", "", "", "neural", "machine", "translation"}); // The empty token produced a visible InvalidRead when launching the tests with Valgrind
  _fuzzyMatcher.sort();

  std::vector<fuzzy::FuzzyMatch::Match> matches;
  ASSERT_NO_THROW(_fuzzyMatcher.match("NMT neural machine translation", 0.1, 1, false, matches));
}

TEST(FuzzyMatchTest, max_tokens_in_pattern) {
  {
    fuzzy::FuzzyMatch fuzzy_matcher(fuzzy::FuzzyMatch::penalty_token::pt_none, 2);
    fuzzy_matcher.add_tm("", "single");
    fuzzy_matcher.add_tm("", "two words");
    fuzzy_matcher.add_tm("", "three kind words");
    fuzzy_matcher.sort();
    fuzzy::export_binarized_fuzzy_matcher(get_temp("tm.fmi"), fuzzy_matcher);
  }

  {
    fuzzy::FuzzyMatch fuzzy_matcher;
    fuzzy::import_binarized_fuzzy_matcher(get_temp("tm.fmi"), fuzzy_matcher);
    EXPECT_EQ(fuzzy_matcher.max_tokens_in_pattern(), 2);

    std::vector<fuzzy::FuzzyMatch::Match> matches;
    fuzzy_matcher.match({"three", "kind", "words"},
                        /*fuzzy=*/1,
                        /*number_of_matches=*/1,
                        matches,
                        /*min_subseq_length=*/3);
    EXPECT_EQ(matches.size(), 0);

    fuzzy_matcher.match({"two", "words"},
                        /*fuzzy=*/1,
                        /*number_of_matches=*/1,
                        matches,
                        /*min_subseq_length=*/2);
    EXPECT_EQ(matches.size(), 1);
  }
}

TEST(FuzzyMatchTest, nfc_normalization) {
  // The ohm character is normalized into omega under NFC normalization.
  const std::string ohm = "Ω";
  const std::string omega = "Ω";

  fuzzy::FuzzyMatch fuzzy_matcher;
  fuzzy_matcher.add_tm("", ohm);
  fuzzy_matcher.sort();

  {
    std::vector<fuzzy::FuzzyMatch::Match> matches;
    fuzzy_matcher.match(ohm,
                        /*fuzzy=*/1,
                        /*number_of_matches=*/1,
                        /*no_perfect=*/false,
                        matches,
                        /*min_subseq_length=*/1);
    EXPECT_EQ(matches.size(), 1);
  }

  {
    std::vector<fuzzy::FuzzyMatch::Match> matches;
    fuzzy_matcher.match(omega,
                        /*fuzzy=*/1,
                        /*number_of_matches=*/1,
                        /*no_perfect=*/false,
                        matches,
                        /*min_subseq_length=*/1);
    EXPECT_EQ(matches.size(), 1);
  }
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  assert(argc == 2);
  data_dir = argv[1];
  temp_dir = fs::temp_directory_path() / fs::unique_path("FuzzyMatchTest-%%%%%_%%%%%");
  fs::create_directory(temp_dir);
  std::cerr << "Temporary directory: " << temp_dir << std::endl;
  return RUN_ALL_TESTS();
}
