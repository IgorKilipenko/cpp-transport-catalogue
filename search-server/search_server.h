#pragma once

#include <functional>
#include <map>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "document.h"
#include "string_processing.h"
#include "paginator.h"

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr const double THRESHOLD = 1e-6;

class SearchServer {
   public:
    using IdsConstIterator = vector<int>::const_iterator;
    using IdsIterator = vector<int>::iterator;

    SearchServer() = default;

    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const string& stop_words_text);

    /// Add new document to the search server's internal database
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings);

    /// Find most matched documents for request
    template <typename T = function<bool(int, DocumentStatus, int)>>
    vector<Document> FindTopDocuments(const string& raw_query, T predicate) const;

    vector<Document> FindTopDocuments(const string& raw_query) const;

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const;

    /// Get total number of documents in internal database
    int GetDocumentCount() const;

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;

    set<std::string> GetStopWords() const;

    IdsConstIterator begin() const;

    IdsConstIterator end() const;

    const map<string, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDuplicates();

   private:
    struct DocumentData {
        int rating = 0;
        DocumentStatus status = DocumentStatus::ACTUAL;
    };
    struct QueryWord {
        string data;
        bool is_minus = false;
        bool is_stop = false;
    };
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, map<string, double>> document_to_words_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids_;
    map<size_t, set<int>> hash_content_;

    bool IsStopWord(const string& word) const;

    vector<string> SplitIntoWordsNoStop(const string& text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    QueryWord ParseQueryWord(string text) const;

    Query ParseQuery(const string& text) const;

    double ComputeWordInverseDocumentFreq(const string& word) const;

    template <typename T = function<bool(int, DocumentStatus, int)>>
    vector<Document> FindAllDocuments(const Query& query, T predicate) const;

    static bool IsValidWord(const string& word);
};

// ----------------------------------------------------------------
// Helper methods
// ----------------------------------------------------------------

/// Exceptions safety version of AddDocument
void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status, const vector<int>& ratings);

/// Exceptions safety version of FindTopDocuments
void FindTopDocuments(const SearchServer& search_server, const string& raw_query);

/// Exceptions safety version of FindTopDocuments
void MatchDocuments(const SearchServer& search_server, const string& query);

/// Pagenate mathing documents
template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

/// Remove all documents duplicates from database
void RemoveDuplicates(SearchServer& search_server);

template <typename TDict, typename TKey>
static auto EraseFromContainer(TKey id, TDict& container) {
    auto ptr = container.find(id);
    if (ptr != container.end()) {
        return container.erase(ptr);
    }
    return container.end();
}

template <typename T>
static typename vector<T>::iterator EraseFromContainer(T id, vector<T>& container) {
    auto ptr = find(container.begin(), container.end(), id);
    if (ptr != container.end()) {
        return container.erase(ptr);
    }
    return container.end();
}

// ----------------------------------------------------------------
// SearchServer template members implementation
// ----------------------------------------------------------------

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename T>
vector<Document> SearchServer::FindTopDocuments(const string& raw_query, T predicate) const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < THRESHOLD) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename T>
vector<Document> SearchServer::FindAllDocuments(const Query& query, T predicate) const {
    map<int, double> document_to_relevance;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}
