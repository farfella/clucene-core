/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "StandardTokenizer.h"
#include "CLucene/util/StringBuffer.h"
#include "CLucene/util/_FastCharStream.h"
#include "CLucene/util/CLStreams.h"

CL_NS_USE(analysis)
CL_NS_USE(util)
CL_NS_DEF2(analysis, standard)

const wchar_t* tokenImageArray[] = {
  L"<EOF>",
  L"<UNKNOWN>",
  L"<ALPHANUM>",
  L"<APOSTROPHE>",
  L"<ACRONYM>",
  L"<COMPANY>",
  L"<EMAIL>",
  L"<HOST>",
  L"<NUM>",
  L"<CJK>"
};
const wchar_t** tokenImage = tokenImageArray;


/* A bunch of shortcut macros, many of which make assumptions about variable
** names.  These macros enhance readability, not just convenience! */
#define EOS           (ch==-1 || rd->Eos())
#define SPACE         (_istspace((wchar_t)ch) != 0)
#define ALPHA         (_istalpha((wchar_t)ch) != 0)
#define ALNUM         (_istalnum(ch) != 0)
#define DIGIT         (_istdigit(ch) != 0)
#define UNDERSCORE    (ch == '_')

#define _CJK			(  (ch>=0x3040 && ch<=0x318f) || \
  						   (ch>=0x3300 && ch<=0x337f) || \
  						   (ch>=0x3400 && ch<=0x3d2d) || \
  						   (ch>=0x4e00 && ch<=0x9fff) || \
  						   (ch>=0xf900 && ch<=0xfaff) || \
  						   (ch>=0xac00 && ch<=0xd7af) ) //korean


#define DASH          (ch == '-')
#define NEGATIVE_SIGN_ DASH
//#define POSITIVE_SIGN_ (ch == '+')
//#define SIGN          (NEGATIVE_SIGN_ || POSITIVE_SIGN_)

#define DOT             (ch == '.')
#define DECIMAL         DOT


//freebsd seems to have a problem with defines over multiple lines, so this has to be one long line
#define _CONSUME_AS_LONG_AS(conditionFails) while (true) { ch = readChar(); if (ch==-1 || (!(conditionFails) || str.length() >= LUCENE_MAX_WORD_LEN)) { break; } str.push_back(ch);}

#define CONSUME_ALPHAS _CONSUME_AS_LONG_AS(ALPHA)

#define CONSUME_DIGITS _CONSUME_AS_LONG_AS(DIGIT)

/* otherMatches is a condition (possibly compound) under which a character
** that's not an ALNUM or UNDERSCORE can be considered not to break the
** span.  Callers should pass false if only ALNUM/UNDERSCORE are acceptable. */
#define CONSUME_WORD                  _CONSUME_AS_LONG_AS(ALNUM || UNDERSCORE)

/*
** Consume CJK characters
*/
#define CONSUME_CJK                   _CONSUME_AS_LONG_AS(_CJK)


/* It is considered that "nothing of value" has been read if:
** a) The "read head" hasn't moved since specialCharPos was established.
** or
** b) The "read head" has moved by one character, but that character was
**    either whitespace or not among the characters found in the body of
**    a token (deliberately doesn't include the likes of '@'/'&'). */
#define CONSUMED_NOTHING_OF_VALUE (rdPos == specialCharPos || (rdPos == specialCharPos+1 && ( SPACE || !(ALNUM || DOT || DASH || UNDERSCORE) )))

#define RIGHTMOST(sb) (sb[sb.length()-1])
#define RIGHTMOST_IS(sb, c) (RIGHTMOST(sb) == c)
/* To discard the last character in a StringBuffer, we decrement the buffer's
** length indicator and move the terminator back by one character. */
#define SHAVE_RIGHTMOST(sb) (sb.pop_back())

//#define REMOVE_TRAILING_CHARS(sb, charMatchesCondition) { wchar_t* sbBuf = sb.getBuffer(); for (int32_t i = sb.len-1; i >= 0; i--) { wchar_t c = sbBuf[i]; if (charMatchesCondition) { sbBuf[--sb.len] = '\0'; } else {break;}}}

/* Does StringBuffer sb contain any of the characters in string ofThese? */
#define CONTAINS_ANY(sb, ofThese) (wcscspn(sb.c_str(), ofThese) != sb.length())


StandardTokenizer::StandardTokenizer(BufferedReader* reader, bool deleteReader) :
    /* rdPos is zero-based.  It starts at -1, and will advance to the first
    ** position when readChar() is first called. */
    rdPos(-1),
    tokenStart(-1),
    rd(_CLNEW FastCharStream(reader))
{
    this->reader = reader;
    this->deleteReader = deleteReader;
}

StandardTokenizer::~StandardTokenizer()
{
    _CLDELETE(rd);
    if (this->deleteReader)
        _CLDELETE(reader)
}

int StandardTokenizer::readChar()
{
    /* Increment by 1 because we're speaking in terms of characters, not
    ** necessarily bytes: */
    rdPos++;
    return rd->GetNext();
}

void StandardTokenizer::unReadChar()
{
    rd->UnGet();
    rdPos--;
}

inline Token* StandardTokenizer::setToken(Token* t, std::wstring & sb, TokenTypes tokenCode)
{
    t->setStartOffset(tokenStart);
    t->setEndOffset(tokenStart + sb.length());
    t->setType(tokenImage[tokenCode]);
    sb.c_str(); //null terminates the buffer
    t->resetTermTextLen();
    return t;
}

void StandardTokenizer::reset(Reader* _input)
{
    this->input = _input;
    if (rd->input == NULL) rd->input = _input->__asBufferedReader();
    rdPos = -1;
    tokenStart = -1;
    rd->reset();
}

Token* StandardTokenizer::next(Token* t)
{
    int ch = 0;

    while (!EOS)
    {
        ch = readChar();

        if (ch == 0 || ch == -1)
        {
            continue;
        }
        else if (SPACE)
        {
            continue;
        }
        else if (ALPHA || UNDERSCORE)
        {
            tokenStart = rdPos;
            t = ReadAlphaNum(ch, t);
            if (t != NULL) return t;
        }
        else if (DIGIT || NEGATIVE_SIGN_ || DECIMAL)
        {
            tokenStart = rdPos;
            /* ReadNumber returns NULL if it fails to extract a valid number; in
            ** that case, we just continue. */
            if (ReadNumber(NULL, ch, t))
                return t;
        }
        else if (_CJK)
        {
            t = ReadCJK(ch, t);
            if (t != NULL) return t;
        }
    }
    return NULL;
}

Token* StandardTokenizer::ReadNumber(const wchar_t* previousNumber, const wchar_t prev, Token* t)
{
    /* previousNumber is only non-NULL if this function already read a complete
    ** number in a previous recursion, yet has been asked to read additional
    ** numeric segments.  For example, in the HOST "192.168.1.3", "192.168" is
    ** a complete number, but this function will recurse to read the "1.3",
    ** generating a single HOST token "192.168.1.3". */
    t->growBuffer(LUCENE_MAX_WORD_LEN + 1);//make sure token can hold the next word
    std::wstring str(t->termBuffer(), t->bufferLength()); //use stringbuffer to read data onto the termText
    TokenTypes tokenType;
    bool decExhausted;
    if (previousNumber != NULL)
    {
        str.insert(0, previousNumber);
        tokenType = CL_NS2(analysis, standard)::HOST;
        decExhausted = false;
    }
    else
    {
        tokenType = CL_NS2(analysis, standard)::NUM;
        decExhausted = (prev == '.');
    }
    if (str.length() >= LUCENE_MAX_WORD_LEN)
    {
        //if a number is too long, i would say there is no point
        //storing it, because its going to be the wrong number anyway?
        //what do people think?
        return NULL;
    }
    str.push_back(prev);

    const bool signExhausted = (prev == '-');
    int ch = prev;

    CONSUME_DIGITS;

    if (str.length() < 2 /* CONSUME_DIGITS didn't find any digits. */
        && (
        (signExhausted && !DECIMAL)
            || (decExhausted /* && !DIGIT is implied, since CONSUME_DIGITS stopped on a non-digit. */)
            )
        )
    {
        /* We have either:
        **   a) a negative sign that's not followed by either digit(s) or a decimal
        **   b) a decimal that's not followed by digit(s)
        ** so this is not a valid number. */
        if (!EOS)
        {
            /* Unread the character that stopped CONSUME_DIGITS: */
            unReadChar();
        }
        return NULL;
    }

    /* We just read a group of digits.  Is it followed by a decimal symbol,
    ** implying that there might be another group of digits available? */
    if (!EOS)
    {
        if (DECIMAL)
        {
            if (str.length() >= LUCENE_MAX_WORD_LEN)
                return NULL; //read above for rationale
            str.push_back(ch);
        }
        else
        {
            unReadChar();
            goto SUCCESSFULLY_EXTRACTED_NUMBER;
        }

        CONSUME_DIGITS;
        if (!DIGIT && !DECIMAL)
        {
            unReadChar();
        }
        else if (!EOS && DECIMAL && _istdigit(rd->Peek()))
        {
            /* We just read the fractional digit group, but it's also followed by
            ** a decimal symbol and at least one more digit, so this must be a
            ** HOST rather than a real number. */
            return ReadNumber(str.c_str(), '.', t);
        }
    }

SUCCESSFULLY_EXTRACTED_NUMBER:
    wchar_t rightmost = RIGHTMOST(str);
    /* Don't including a trailing decimal point. */
    if (rightmost == '.')
    {
        SHAVE_RIGHTMOST(str);
        unReadChar();
        rightmost = RIGHTMOST(str);
    }
    /* If all we have left is a negative sign, it's not a valid number. */
    if (rightmost == '-')
    {
        CND_PRECONDITION(str.length() == 1, L"Number is invalid");
        return NULL;
    }

    return setToken(t, str, tokenType);
}

Token* StandardTokenizer::ReadAlphaNum(const wchar_t prev, Token* t)
{
    t->growBuffer(LUCENE_MAX_WORD_LEN + 1);//make sure token can hold the next word
    std::wstring str(t->termBuffer(), t->bufferLength()); //use stringbuffer to read data onto the termText
    if (str.length() < LUCENE_MAX_WORD_LEN)
    {
        str.push_back(prev);
        int ch = prev;

        CONSUME_WORD;
        if (!EOS && str.length() < LUCENE_MAX_WORD_LEN - 1)
        { //still have space for 1 more character?
            switch (ch)
            { /* What follows the first alphanum segment? */
            case '.':
                str.push_back('.');
                return ReadDotted(str, CL_NS2(analysis, standard)::UNKNOWN, t);
            case '\'':
                str.push_back('\'');
                return ReadApostrophe(str, t);
            case '@':
                str.push_back('@');
                return ReadAt(str, t);
            case '&':
                str.push_back('&');
                return ReadCompany(str, t);
                /* default: fall through to end of this function. */
            }
        }
    }
    return setToken(t, str, CL_NS2(analysis, standard)::ALPHANUM);
}

Token* StandardTokenizer::ReadCJK(const wchar_t prev, Token * t)
{
    t->growBuffer(LUCENE_MAX_WORD_LEN + 1);//make sure token can hold the next word
    std::wstring str(t->termBuffer(), t->bufferLength()); //use stringbuffer to read data onto the termText
    if (str.length() < LUCENE_MAX_WORD_LEN)
    {
        str.push_back(prev);
        int ch = prev;

        CONSUME_CJK;
    }

    return setToken(t, str, CL_NS2(analysis, standard)::CJK);
}


Token * StandardTokenizer::ReadDotted(std::wstring & str, TokenTypes forcedType, Token * t)
{
    const int32_t specialCharPos = rdPos;

    /* A segment of a "dotted" is not allowed to begin with another dot or a dash.
    ** Even though hosts, e-mail addresses, etc., could have a dotted-segment
    ** that begins with a dot or a dash, it's far more common in source text
    ** for a pattern like "abc.--def" to be intended as two tokens. */
    int ch = rd->Peek();
    if (!(DOT || DASH))
    {
        bool prevWasDot;
        bool prevWasDash;
        if (str.length() == 0)
        {
            prevWasDot = false;
            prevWasDash = false;
        }
        else
        {
            prevWasDot = RIGHTMOST(str) == '.';
            prevWasDash = RIGHTMOST(str) == '-';
        }
        while (!EOS && str.length() < LUCENE_MAX_WORD_LEN - 1)
        {
            ch = readChar();
            const bool dot = ch == '.';
            const bool dash = ch == '-';

            if (!(ALNUM || UNDERSCORE || dot || dash))
            {
                break;
            }
            /* Multiple dots or dashes in succession end the token.
            ** Consider the following inputs:
            **   "Visit windowsupdate.microsoft.com--update today!"
            **   "In the U.S.A.--yes, even there!"                 */
            if ((dot || dash) && (prevWasDot || prevWasDash))
            {
                /* We're not going to append the character we just read, in any case.
                ** As to the character before it (which is currently RIGHTMOST(str)):
                ** Unless RIGHTMOST(str) is a dot, in which we need to save it so the
                ** acronym-versus-host detection can work, we want to get rid of it. */
                if (!prevWasDot)
                {
                    SHAVE_RIGHTMOST(str);
                }
                break;
            }

            str.push_back(ch);

            prevWasDot = dot;
            prevWasDash = dash;
        }
    }

    /* There's a potential StringBuffer.append call in the code above, which
    ** could cause str to reallocate its internal buffer.  We must wait to
    ** obtain the optimization-oriented strBuf pointer until after the initial
    ** potentially realloc-triggering operations on str.
    ** Because there can be other such ops much later in this function, strBuf
    ** is guarded within a block to prevent its use during or after the calls
    ** that would potentially invalidate it. */
    { /* Begin block-guard of strBuf */
        const wchar_t* strBuf = str.c_str();

        bool rightmostIsDot = RIGHTMOST_IS(str, '.');
        if (CONSUMED_NOTHING_OF_VALUE)
        {
            /* No more alphanums available for this token; shave trailing dot, if any. */
            if (rightmostIsDot)
            {
                SHAVE_RIGHTMOST(str);
            }
            /* If there are no dots remaining, this is a generic ALPHANUM. */
            if (wcschr(strBuf, '.') == NULL)
            {
                forcedType = CL_NS2(analysis, standard)::ALPHANUM;
            }

            /* Check the token to see if it's an acronym.  An acronym must have a
            ** letter in every even slot and a dot in every odd slot, including the
            ** last slot (for example, "U.S.A."). */
        }
        else if (rightmostIsDot)
        {
            bool isAcronym = true;
            const int32_t upperCheckLimit = str.length() - 1; /* -1 b/c we already checked the last slot. */

            for (int32_t i = 0; i < upperCheckLimit; i++)
            {
                const bool even = (i % 2 == 0);
                ch = strBuf[i];
                if ((even && !ALPHA) || (!even && !DOT))
                {
                    isAcronym = false;
                    break;
                }
            }
            if (isAcronym)
            {
                forcedType = CL_NS2(analysis, standard)::ACRONYM;
            }
            else
            {
                /* If it's not an acronym, we don't want the trailing dot. */
                SHAVE_RIGHTMOST(str);
                /* If there are no dots remaining, this is a generic ALPHANUM. */
                if (wcschr(strBuf, '.') == NULL)
                {
                    forcedType = CL_NS2(analysis, standard)::ALPHANUM;
                }
            }
        }
    } /* End block-guard of strBuf */

    if (!EOS)
    {
        if (ch == '@' && str.length() < LUCENE_MAX_WORD_LEN - 1)
        {
            str.push_back('@');
            return ReadAt(str, t);
        }
        else
        {
            unReadChar();
        }
    }

    return setToken(t, str, CL_NS2(analysis, standard)::UNKNOWN
        ? forcedType : CL_NS2(analysis, standard)::HOST);
}

Token* StandardTokenizer::ReadApostrophe(std::wstring & str, Token* t)
{

    TokenTypes tokenType = CL_NS2(analysis, standard)::APOSTROPHE;
    const int32_t specialCharPos = rdPos;
    int ch = 0;

    CONSUME_ALPHAS;
    if (RIGHTMOST_IS(str, '\'') || CONSUMED_NOTHING_OF_VALUE)
    {
        /* After the apostrophe, no more alphanums were available within this
        ** token; shave trailing apostrophe and revert to generic ALPHANUM. */
        SHAVE_RIGHTMOST(str);
        tokenType = CL_NS2(analysis, standard)::ALPHANUM;
    }
    if (!EOS)
    {
        unReadChar();
    }

    return setToken(t, str, tokenType);
}

Token* StandardTokenizer::ReadAt(std::wstring & str, Token* t)
{
    ReadDotted(str, CL_NS2(analysis, standard)::EMAIL, t);
    /* JLucene grammar indicates dots/digits not allowed in company name: */
    if (!CONTAINS_ANY((str), L".0123456789"))
    {
        setToken(t, str, CL_NS2(analysis, standard)::COMPANY);
    }
    return t;
}

Token* StandardTokenizer::ReadCompany(std::wstring & str, Token* t)
{
    const int32_t specialCharPos = rdPos;
    int ch = 0;

    CONSUME_WORD;
    if (CONSUMED_NOTHING_OF_VALUE)
    {
        /* After the ampersand, no more alphanums were available within this
        ** token; shave trailing ampersand and revert to ALPHANUM. */
        CND_PRECONDITION(RIGHTMOST_IS(str, '&'), L"ReadCompany failed");
        SHAVE_RIGHTMOST(str);


        return setToken(t, str, CL_NS2(analysis, standard)::ALPHANUM);
    }
    if (!EOS)
    {
        unReadChar();
    }

    return setToken(t, str, CL_NS2(analysis, standard)::COMPANY);
}

CL_NS_END2
