#pragma once

#include <boost/serialization/vector.hpp>

#include <fuzzy/suffix_array_index.hh>
#include <fuzzy/sentence.hh>
#include <fuzzy/edit_distance.hh>

namespace onmt {
  class Tokenizer;
}

namespace fuzzy
{
  class FuzzyMatch
  {
  public:
    static const char version;

    enum penalty_token {
      pt_none = 0,
      pt_tag = 1 << 0,
      pt_pct = 1 << 1,
      pt_sep = 1 << 2,
      pt_jnr = 1 << 3,
      pt_nbr = 1 << 4,
      pt_cas = 1 << 5,
    };

    struct Match
    {
      float       score;
      int         max_subseq;
      unsigned    s_id;
      std::string id;
    };

    FuzzyMatch(int pt = penalty_token::pt_none,
               size_t max_tokens_in_pattern = SuffixArrayIndex::DEFAULT_MAX_TOKENS_IN_PATTERN);
    ~FuzzyMatch();

    bool add_tm(const std::string& id, const Tokens& norm, bool sort = true);
    bool add_tm(const std::string& id, const Sentence& source, const Tokens& norm, bool sort = true);
    /* integrated tokenization */
    bool add_tm(const std::string& id, const std::string &sentence, bool sort = true);

    void sort();
    /* backward compatibility */
    bool match(const Tokens& pattern,
               float fuzzy,
               unsigned number_of_matches,
               std::vector<Match> &matches,
               int min_subseq_length=2,
               float min_subseq_ratio=0,
               float vocab_idf_penalty=0) const;
    bool match(const Sentence& real,
               const Tokens& pattern,
               float fuzzy,
               unsigned number_of_matches,
               bool no_perfect,
               std::vector<Match>& matches,
               int min_subseq_length=3,
               float min_subseq_ratio=0.3,
               float vocab_idf_penalty=0) const;
    /* simplified, include tokenization */
    bool match(const std::string &sentence,
               float fuzzy,
               unsigned number_of_matches,
               bool no_perfect,
               std::vector<Match>& matches,
               int min_subseq_length=3,
               float min_subseq_ratio=0.3,
               float vocab_idf_penalty=0) const;
    bool subsequence(const std::string &sentence,
               unsigned number_of_matches,
               bool no_perfect,
               std::vector<Match>& matches,
               int min_subseq_length=3,
               float min_subseq_ratio=0.3,
               bool idf_weighting=false) const;
    /* tokenize and normalize a sentence - with options defined when creating the
       fuzzyMatcher */
    void _tokenize_and_normalize(const std::string &sentence,
                                 Sentence& real,
                                 Tokens& pattern) const;
    /* tokenize and normalize a sentence, and preserve the actual 
       tokens and features list, together with mapping between ID in the sentence
       and original token IDs */
    void _tokenize_and_normalize(const std::string &sentence,
                                 Sentence& real,
                                 Tokens& pattern,
                                 std::vector<unsigned> &map_tokens,
                                 std::vector<std::string> &tokens,
                                 std::vector<std::vector<std::string>> &features) const;
    std::ostream& dump(std::ostream& os) const;

    size_t max_tokens_in_pattern() const;

  private:
    friend class boost::serialization::access;
    friend void import_binarized_fuzzy_matcher(const std::string& binarized_tm_filename, FuzzyMatch& fuzzy_matcher);

    void _update_tokenizer();

    template<class Archive>
    void save(Archive&, unsigned int version) const;

    template<class Archive>
    void load(Archive&, unsigned int version);

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    float compute_max_idf_penalty() const;

    std::vector<float>
    compute_idf_penalty(const std::vector<unsigned int>& pattern_wids,
                        float unknown_vocab_word_penalty = 0) const;

    /* penalty tokens */
    int                    _pt;
    /* open-nmt tokenizer */
    std::unique_ptr<onmt::Tokenizer> _ptokenizer;
    /* Suffix-Array Index */
    std::unique_ptr<SuffixArrayIndex> _suffixArrayIndex;
  };
}

# include <fuzzy/fuzzy_match.hxx>
