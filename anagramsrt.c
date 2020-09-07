// 09/05/2020 4X speedup
// sort word list by length so that search traverses just once
// use this to generate a word list sorted by length:
// cat enable1.txt| perl -nlpe 's/^/100+length($_)/e'|sort|cut -b4-99 > enable1.srtlen
// 7.7 ms vs. 31.7 ms, so about 4X speedup. Use this for timing:
// echo #######,ideation > ideation
// perl -e 'for(1..100){print `./anagram < ideation`}' 

/* anagram.c 
 * Copyright (C) 2004 by aaron w west
 * tallpeak@hotmail.com / awilliamwest@yahoo.com
 * 11/27/2004 to 11/30
 * bugs: 1) entering a new word should clear the pattern
 *       2) pattern should be implicitly added to the search string
 *       3) output looks very messy (wraps at end of line)
 * A simple scrabble/literati word finder
 *
 * Source of enable1 word list:
 * http://www.puzzlers.org/wordlists/dictinfo.php
 * Current timings (timeit) on Athlon 2400 XP laptop: 
 * 0.7 seconds to init & load the dictionary
 * 3.4 seconds for 100 dictionary scans (100 Xs) including init
 * IE 27 ms per scan
 * Todo: 
 * 1) Optimizations:
 *  a) Prepare the search term counts once per dictionary scan
 *    - implemented as static buffers (not threadsafe) & strcmp
 *  b) Collect the results in a list/array and sort by length
 *    - an alternative would be to bucketize the dictionary words 
 *    - putting all same-length ones together
 *    - scan the prefix-compressed file twice (in mem)
 *    - once to get the count of words of each length
 *    - second to fill the arrays
 *  c) Collect the dictionary words into a linked list (stemmed?)
 *    - done, each word has two byte lengths preceding the suffix
 *    - as follows (prefixlen)(suffixlen)char suffix[suffixlen]
 *  d) Build a hashtable of possible word combinations based on sorted letters
 *   - this would help optimize certain cases. Questionable value...
 * 2) Show all bingo words first/last (easily seen together)?
 *   - the hashtable could be used for this, showing these a full
 *   - tenth of a second before the dictionary scan...
 * 3) Support required substrings, begin/middle/end (caps are a required substring?) abcdefg+x_yz
 *   - mostly implemented, just need to change substr to support blanks
 *   - and possibly wildcards (globbing patterns), eg ab?d??g*xyz
 * 4) Convert to Win32 GUI? (ForeWord-type interface?)
 *   a) show words that can be made with rack letters alone, sorted by word
 *   b) show words that can be possibly connected to a single letter
 *      on the board, sorted by connecting letter, and positioned 
 *      with the connecting letters lined up on the screen
 * 5) CGI interface? (like a2zwordfinder) ?
 * 6) Multi-user optimization? Queue the words and perform searches in parallel (yeah sure...well, if I ever wanted to support more than 100 searches per second... ask a2zwordfinder.com if they want to pay for it...)
 * cl /Ox /G7 anagram.c
 * Timings for a search of 100 xs (100 dictionary scans)
 * timing before prefix compression: 3.21 seconds min of 4.
 * timing after  prefix compression: 2.35 seconds min of 4. 
 * timing after search term caching: 2.02 seconds min of 4. 
 * Initialization time: 0.40 seconds
 * Time per dictionary scan is ~ 16 ms
 * Time per word with min 4 for 1000 words is 114 ms 

How to generate the prefix-compressed file:
# stem.pl
# compress enable1.txt by "stemming" - just remove the common prefix for now
#
use strict;
open IF, "enable1.txt" || die;
open OF, ">enable1.stm" || die;
my $prevword='';
my $diff;
my $word;
my $suffix;
my $countsame;
my $linecount;
while(defined ($word = <IF>) && ++$linecount)# < 20)
    {
        $word =~ tr/\r\n//d;
        $_ = $word ^ $prevword;
        s/^([\0]+)//;
        $countsame = length $1;
        $suffix = substr($word, $countsame);
        print OF chr $countsame, chr length $suffix, $suffix;
#Format for static .c file, but doesnt work with VC++, max 65535 string literal error:
#	printf OF2 "\"\\%o\\%o%s\"\n", $countsame, length $suffix, $suffix;
        $prevword = $word;
    }
print OF "\0\0";
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// C11 breaks strcpy & strncpy for deleting characters from a buffer (overlapping)
// https://developer.apple.com/forums/thread/86895

// this worked, though it's a crude hack:
//#undef strcpy
//char *mystrcpy(void *dest, void *source)
//{
//    size_t sourceLen = strlen(source);
//    memcpy(dest, source, sourceLen);
//    *((char*)dest + sourceLen) = '\0'; //null terminate
//    return dest;
//}
//#define strcpy mystrcpy

// Better is:
void *deleteChar(char *p)
{
    while (*p) {
        *p++ = p[1];
    }
    *p = '\0';
}

/* gcc works but MS VC++ has a 65535 byte limit on literal string constants:
#include "enable1.c"
 */

#define BEGIN_WORD_FLAG 1
#define END_WORD_FLAG 2

int isalpha(int c)
{
    return ((unsigned) c | 32) - 97 < 26;
}

void makelower(unsigned char *d)
{
    unsigned int c;
    for(;(c = *d) != '\0';d++) 
    {
        if ((c-'A')<26)
            *d |= 32;
    }
}

//char *dictfile = "enable1.stm";

// char dictbuf[1916187];
 // prefix-compressed it's shorter
//#define DICTSIZE 733537
int dictsize = 0;
//char *dictbuf = NULL;
/* return 1 if the dictionary word matches.
 * return 0 if the dictionary word contains more non-matching letters
 * than search-term blanks
 */
int matchword(unsigned char *dictword, 
    unsigned char *search, unsigned char *pattern, 
    int flags, int minlen, int maxlen)
{
    int blanks = 0;
    register unsigned c;
    unsigned char *s;
    register unsigned char *d;
    int i;
    static unsigned char prevsearch[257] = "";
    static int capsfound = 0;
    static int initblanks = 0;
    static unsigned int lowercounts[26]; // lowercase letter counts in search
    static unsigned int uppercounts[26];
    unsigned int searchletters[26];
    unsigned int capletters[26];
    int l = strlen(dictword);
    if (l < minlen || l > maxlen) 
    {
        return 0;
    }
    if (pattern[0])
    {
        s = strstr(dictword, pattern);
        if (!s || (flags & BEGIN_WORD_FLAG) && s != dictword
              || (flags & END_WORD_FLAG)
                    && s != dictword + strlen(dictword) - strlen(pattern))
            return 0;
    }
    if (strcmp(search, prevsearch) != 0)
    {
        memset(lowercounts, 0, sizeof(lowercounts));
        memset(uppercounts, 0, sizeof(uppercounts));
        initblanks = 0;
        capsfound = 0;
        strcpy(prevsearch, search);
    /* first count blanks, # or ?, and each letter */
        for (s = search; (c = *s) != '\0'; s++)
        {
            c -= 'A';
            if (c < 26) {
                capsfound++;
                uppercounts[c]++;
                lowercounts[c]++;
            } else {
                c -= 32;
                if (c < 26)
                    lowercounts[c]++;
                else if ((c = *s) == '?' || c == '#') 
                    initblanks++;
            }
        }
    }
    memcpy(searchletters, lowercounts, sizeof(lowercounts));
    memcpy(capletters, uppercounts, sizeof(uppercounts));
    blanks = initblanks;

    for (d = dictword; (c = *d - 'a') < 26; d++)
    {
        if (searchletters[c] > 0) 
        {
            searchletters[c]--;
        } else {
            if (blanks > 0)
            {
                *d -= 32; // *d = toupper(*d)
                blanks--;
            } else {
                return 0;
            }
        }
        if (capletters[c] > 0)
            capletters[c]--;
    }
    //check for caps=required letters in search
    if (capsfound)
    {
        for (c = 0; c < 26; c++)
            if (capletters[c] > 0)
                return 0; // a cap letter specified wasnt used
    }
    return 1;
}

/* dump all matched words to stdout */
int scandict(unsigned char *search, unsigned char *pattern, int flags, 
             unsigned char *dictbuf, int minlen, int maxlen)
{
    unsigned char dictword[257] = ""; // should be more than enough 
    unsigned char outword[257] = ""; 
    unsigned char *p = dictbuf, *e, *nextp;
    int wordcount = 0;
    int totalcount = 0;
    int prevlen = 0;
    int wordlen = 0;
    while(p < dictbuf + dictsize && wordlen <= maxlen)
    {
        e = strchr(p, '\n');
        if (!e) 
            break;
        nextp = e+1;
        while (e[-1] <= ' ' && e > p) 
            e--;
        wordlen = e-p;
        strncpy(dictword, p, wordlen);
        dictword[wordlen] = '\0';
    
        if (wordlen != prevlen) {
            if (wordcount) {
                printf("(%d)| ", wordcount);
                totalcount += wordcount;
                wordcount = 0;
            }
            else
                printf("-| ");
            if (wordlen > maxlen) 
                break;
            printf("%d:", wordlen);
            prevlen = wordlen;
        }

        //printf("%s\n", dictword);
        strcpy(outword, dictword);
        if (matchword(outword, search, pattern, flags, minlen, maxlen)) {
            wordcount++;
            printf("%s ", outword);
        }
        p=nextp;
    }
    printf("(Total:%d)\n", totalcount);
    return totalcount;
}

int main(int argc, char *argv[])
{
    unsigned char wordbuf[128] = "RackLetters+-#", inbuf[128], *p;
    FILE *dictfp;
    int length, l, i, thresh = 4;
    int wordcount, totalcount;
    unsigned c;
    unsigned char *dictbuf;
    unsigned char pattern[128] = "";
    unsigned char patbuf[128] = "";
    int flags;
    unsigned char *dictfile = "enable1.srtlen"; 
    if (argc > 1 && argv[1][0])
        dictfile = argv[1];
    if (dictfile)
    {
        dictfp = fopen(dictfile, "rb");
        if (dictfp == NULL)
        {
            printf("Can't open dictionary file %s, check to make sure it's in the working directory. Exiting.\n", dictfile);
            return 1;
        }
        // file size = position at end
        fseek(dictfp, 0, SEEK_END);
        dictsize = ftell(dictfp); 
        fseek(dictfp, 0, SEEK_SET);//rewind
        dictbuf = (unsigned char *)malloc(dictsize + 2);
        fread(dictbuf, 1, dictsize, dictfp);
        dictbuf[dictsize] = '\0';
        dictbuf[dictsize + 1] = '\0';
        fclose(dictfp);
    } else {
        printf("VC++ doesn't support > 64k literal strings, dying\n");
        return 1;
        /* dictbuf = static_dictbuf; */
    }
    while(printf("[%s,%s]:", wordbuf, pattern) 
          && fgets(inbuf, sizeof(inbuf)-1, stdin))
    {
        l = strlen(inbuf);
        while (inbuf[l-1] <= ' ' && l > 0) 
            l--; 
        inbuf[l] = '\0';
        if (inbuf[1] == '\0' && (isalpha(*inbuf) || *inbuf == '#') )
        {
            strcat(wordbuf, inbuf);
        } else if (inbuf[0] == ',') {
            strcpy(pattern, inbuf+1);
        } else if (inbuf[0] == '+') {
            strcat(wordbuf, inbuf+1);
        } else if (inbuf[0] == '-') {
            for (i = 0; inbuf[i] == '-'; i++) 
                ;
            p = wordbuf + strlen(wordbuf) - i;
            if (p < wordbuf)  {
                p = wordbuf;
            }
            strcpy(p, inbuf+i);
            /* strcpy(wordbuf + strlen(wordbuf) - 1, inbuf+1); */
        } else if (!strcmp(inbuf, "X")) 
            break;
        else if (inbuf[0] == '\0') {
            if (strlen(wordbuf) > 8) 
            {
                wordbuf[8] = '\0';
            } else if (p = strchr(wordbuf, '#'))
            {
                //strcpy(p, p+1); // BUG!
                deleteChar(p); // avoids __os_crash (due to overlapping buffers)
            } else {
                printf(" Enter the letters from your rack, X to exit.\n"
                       " Use upper-case to specify required letters, # for blanks.\n"
                       " Enter a digit (2=9) as part of your search to specify\n"
                       " the minimum word length found (default 4, now %d).\n"
                       "# alone adds a blank to your search\n"
                       " Press enter alone to remove anything after character 7,\n"
                       " or to delete a blank from your search word.\n"
                       "+letters to add letters to your search\n"
                       "-letters to add at the last character (minus alone deletes last char).\n"
                       ",pattern to specify board letters that must appear in the specified order\n"
                       ",<enter>, or enter a new word, to clear the pattern\n"
                       "^pattern matches beginning, pattern$ matches end of word. \n"
                       " Spaces in the pattern are not (yet) supported.\n"
                       "Results will be ordered by length\n", thresh);
                continue;
            }
        } else {
            pattern[0] = '\0';
            strcpy(wordbuf, inbuf);
        }
        l = strlen(wordbuf);
        for (i = 0; c = wordbuf[i] - '0', i < l; i++)
        {
            if (c < 10)
                thresh = c > 2 ? c : 2;
        }
        p = strchr(wordbuf, ',');
        if (p) 
        {
            deleteChar(p);
            *p = '\0';
            l = p - wordbuf;
            makelower(pattern);
        }
        totalcount = 0;
        flags = 0;
        strcpy(patbuf, pattern);
        p = strchr(patbuf, '$');
        if (p) {
            flags |= END_WORD_FLAG;
            deleteChar(p);
        }
        p = strchr(patbuf, '^');
        if (p) {
            flags |= BEGIN_WORD_FLAG;
            deleteChar(p);
        }
        int minlen = l < thresh ? l : thresh;
        int maxlen = l;
        scandict(wordbuf, patbuf, flags, dictbuf, minlen, maxlen);
    }
    return 0;
}
