
#include "tokenizer.h"

using namespace std;
using namespace boost::filesystem;

vector< string > tokenize(const string &in) {

  vector< string > toks;
  bool white = true;
  size_t begin = 0;
  for (size_t i = 0; i <= in.size(); i++) {
    char c = (i < in.size()) ? in[i] : ' ';
    if (is_mm_valid(c)) {
      if (white) {
        begin = i;
        white = false;
      }
    } else if (is_mm_whitespace(c)) {
      if (!white) {
        toks.push_back(in.substr(begin, i-begin));
        white = true;
      }
    } else {
      throw MMPPParsingError("Wrong input character");
    }
  }
  return toks;

}

/*FileTokenizer::FileTokenizer(const string &filename) :
    fin(filename), base_path(path(filename).parent_path()), cascade(nullptr), white(true)
{
}*/

FileTokenizer::FileTokenizer(const path &filename, Reportable *reportable) :
    fin(filename), base_path(filename.parent_path()), cascade(nullptr), white(true), reportable(reportable)
{
    this->set_file_size();
}

FileTokenizer::FileTokenizer(string filename, path base_path) :
    fin(filename), base_path(base_path), cascade(nullptr), white(true), reportable(nullptr)
{
    this->set_file_size();
}

void FileTokenizer::set_file_size()
{
    //return;
    auto pos = this->fin.tellg();
    if (pos < 0) {
        return;
    }
    this->fin.seekg(0, ios_base::end);
    this->filesize = this->fin.tellg();
    this->fin.seekg(pos, ios_base::beg);
    if (this->reportable != nullptr && this->filesize > 0) {
        this->reportable->set_total((double) this->filesize);
    }
}

char FileTokenizer::get_char()
{
    char c;
    this->fin.get(c);
    this->pos++;
    if (this->reportable != nullptr) {
        this->reportable->report((double) this->pos);
    }
    return c;
}

std::pair<bool, string> FileTokenizer::finalize_token(bool comment) {
    if (this->white) {
        return make_pair(comment, "");
    } else {
        string res = string(this->buf.begin(), this->buf.end());
        this->buf.clear();
        this->white = true;
        return make_pair(comment, res);
    }
}

std::pair<bool, string> FileTokenizer::next()
{
    while (true) {
        if (this->cascade != nullptr) {
            auto next_pair = this->cascade->next();
            if (next_pair.second != "") {
                return next_pair;
            } else {
                delete this->cascade;
                this->cascade = nullptr;
            }
        }
        char c = this->get_char();
        if (this->fin.eof()) {
            return this->finalize_token(false);
        }
        if (c == '$') {
            if (!this->white) {
                throw MMPPParsingError("Dollars cannot appear in the middle of a token");
            }
            // This can be a regular token or the beginning of a comment
            c = this->get_char();
            if (this->fin.eof()) {
                throw MMPPParsingError("Interrupted dollar sequence");
            }
            if (c == '(' || c == '[') {
                // Here the comment begin
                bool found_dollar = false;
                bool comment = c == '(';
                vector< char > content;
                while (true) {
                    c = this->get_char();
                    if (this->fin.eof()) {
                        throw MMPPParsingError("File ended in comment or in file inclusion");
                    }
                    if (found_dollar) {
                        if ((comment && c == '(') || (!comment && c == '[')) {
                            throw MMPPParsingError("Comment and file inclusion opening forbidden in comments and file inclusions");
                        } else if ((comment && c == ')') || (!comment && c == ']')) {
                            this->white = true;
                            if (comment) {
                                if (content.empty()) {
                                    break;
                                } else {
                                    return make_pair(true, string(content.begin(), content.end()));
                                }
                            } else {
                                string filename = trimmed(string(content.begin(), content.end()));
                                string actual_filename = (this->base_path / filename).string();
                                this->cascade = new FileTokenizer(actual_filename, this->base_path);
                                break;
                            }
                        } else if (c == '$') {
                            content.push_back('$');
                        } else {
                            content.push_back('$');
                            content.push_back(c);
                            found_dollar = false;
                        }
                    } else {
                        if (c == '$') {
                            found_dollar = true;
                        } else {
                            content.push_back(c);
                        }
                    }
                }
            } else if (c == ')') {
                throw MMPPParsingError("Comment closed while not in comment");
            } else if (c == ']') {
                throw MMPPParsingError("File inclusion closed while not in comment");
            } else if (c == '$' || is_mm_valid(c)) {
                this->buf.push_back('$');
                this->buf.push_back(c);
                this->white = false;
            } else if (is_mm_whitespace(c)) {
                throw MMPPParsingError("Interrupted dollar sequence");
            } else {
                throw MMPPParsingError("Forbidden input character");
            }
        } else if (is_mm_valid(c)) {
            this->buf.push_back(c);
            this->white = false;
        } else if (is_mm_whitespace(c)) {
            if (!this->white) {
                return this->finalize_token(false);
            }
        } else {
            throw MMPPParsingError("Forbidden input character");
        }
    }
}

FileTokenizer::~FileTokenizer()
{
    delete this->cascade;
}

TokenGenerator::~TokenGenerator()
{
}
