#include <unicode/ustream.h>
#include "basic_tokenizer.h"
#include <unicode/schriter.h>
#include <unicode/brkiter.h>

BasicTokenizer::BasicTokenizer(bool doLowerCase) : doLowerCase (doLowerCase) {};


std::vector<std::string> BasicTokenizer::tokenize(icu::UnicodeString &s) const {
  s = clean(s);
  s = tokenizeCJKChars(s);
  s = s.trim();

  std::vector<icu::UnicodeString> origTokens = whitespaceTokenize(s);
  std::vector<icu::UnicodeString> splitToken, splitTokens;

  // Split each token in word pieces (`splitToken`) and then collect all the
  // word pieces together (`splitTokens`)
  for (icu::UnicodeString& token : origTokens) {
    if (token == "[SEP]") {
      // Don't split that
      splitTokens.push_back(token);
      continue;
    }
    if (doLowerCase) token = token.toLower();
    token = stripAccents(token);
    splitToken = splitPunctuation(token);
    splitTokens.insert(splitTokens.end(), splitToken.begin(), splitToken.end());
  }
  return toStdString(splitTokens);
};

std::vector<std::string> BasicTokenizer::toStdString(const std::vector<icu::UnicodeString>& ss) {
  std::vector<std::string> os;
  std::string o;
  for (const auto& s : ss) {
    s.toUTF8String(o);
    os.push_back(o);
    o.clear();
  }
  return os;
}

icu::UnicodeString BasicTokenizer::clean(const icu::UnicodeString &i) const {
  icu::UnicodeString o;
  const UChar *iBuffer = i.getBuffer();
  UCharCharacterIterator it(iBuffer, i.length());
  UChar32 c;
  while (it.hasNext()) {
    c = it.next32PostInc();
    if (c == 0 || c == 0xfffd || u_iscntrl(c)) {
      // Remove invalid and control characters
      continue;
    } else if (u_isspace(c)) {
      // Replace any space with a standard one
      o += ' ';
    } else {
      o += c;
    }
  }
  return o;
};

icu::UnicodeString BasicTokenizer::tokenizeCJKChars(const icu::UnicodeString &i) const {
  icu::UnicodeString o;
  const UChar *iBuffer = i.getBuffer();
  UCharCharacterIterator it(iBuffer, i.length());
  UChar32 c;
  while (it.hasNext()) {
    c = it.next32PostInc();
    if (   (c >= 0x4e00  && c <= 0x9fff) 
        || (c >= 0x3400  && c <= 0x4dbf) 
        || (c >= 0x20000 && c <= 0x2a6df) 
        || (c >= 0x2a700 && c <= 0x2b73f) 
        || (c >= 0x2b740 && c <= 0x2b81f) 
        || (c >= 0x2b820 && c <= 0x2ceaf) 
        || (c >= 0xf900  && c <= 0xfaff) 
        || (c >= 0x2f800 && c <= 0x2fa1f)) {
      // Is a CJK character, enclose in spaces (tokenized)
      o += " ";
      o += c;
      o += + " ";
    } else {
      o += c;
    }
}
return o;
}

std::vector<icu::UnicodeString> BasicTokenizer::whitespaceTokenize(const icu::UnicodeString &s) const {
  icu::UnicodeString t;
  std::vector<icu::UnicodeString> o;
  const UChar *sBuffer = s.getBuffer();
  UCharCharacterIterator it(sBuffer, s.length());
  UChar32 c;
  while (it.hasNext()) {
    c = it.next32PostInc();
    if (u_isspace(c)) {
      // Space character, push_back last word and reset the string
      o.push_back(t);
      t.remove();
    } else {
      t += c;
    }
  }
  if (t.length() > 0) {
    // Append the last word
    o.push_back(t);
  }
  return o;
}

icu::UnicodeString BasicTokenizer::stripAccents(const icu::UnicodeString &i) const {
  icu::UnicodeString o;
  const UChar *iBuffer = i.getBuffer();
  UCharCharacterIterator it(iBuffer, i.length());
  UChar32 c;
  while (it.hasNext()) {
    c = it.next32PostInc();
    if (u_charType(c) == U_NON_SPACING_MARK) {
      // Ignore accent character
      continue;
   } else {
      o += c;
    }
  }
  return o;
}

std::vector<icu::UnicodeString> BasicTokenizer::splitPunctuation(const icu::UnicodeString s) const {
  icu::UnicodeString t;
  std::vector<icu::UnicodeString> o;
  const UChar *sBuffer = s.getBuffer();
  UCharCharacterIterator it(sBuffer, s.length());
  UChar32 c;
  while (it.hasNext()) {
    c = it.next32PostInc();
    if (u_ispunct(c)
        || (c >= 33  && c <= 47)
        || (c >= 58  && c <= 64)
        || (c >= 91  && c <= 96)
        || (c >= 123 && c <= 126)) {
      // Is a punctuation or symbol character, append the word and the character
      // enclosed in spaces (tokenized)
      o.push_back(t);
      t.remove();
      t = c;
      o.push_back(t);
      t.remove();
    } else {
      t += c;
    }
  }
  if (t.length() > 0) {
    o.push_back(t);
  }
  return o;
}
