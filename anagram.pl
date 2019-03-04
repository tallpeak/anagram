#anagram.pl by aaron west 11/24/2004 tallpeak@hotmail.com
# The enable1 word list is here:
# http://www.puzzlers.org/wordlists/dictinfo.php
# download it and put it in a path and edit the line below:

my $wordfile = "c:/dl2/dictionaries/enable1.txt";

# This program builds a hashtable where the key is 
# the sorted characters of the word and the value
# is an ref to an array of all words that can be made 
# with that set of letters.

# For example (the three most "anagrammatic" words, if that makes sense 
#  (I'd say anagrammable, but that's not a word)):
# my %words = ('aeprs', [ qw(apers apres asper pares parse pears prase presa rapes reaps spare spear)],
# 'aelrst', [qw(alerts alters artels estral laster ratels salter slater staler stelar talers,
# 'aelst', [qw(least setal slate stale steal stela taels tales teals tesla)]);

use strict;

sub sortword($) {
    $_ = join "", sort ( split //, shift );
}

my %words;
my $wordcount = 0;

sub readwords {

 #read words into a hashtable where the index is the word in letter-sorted-order
 #and each element is an array of words spelled with those letters.
    open FH, $wordfile || die "Can't open dictionary";
    my @words = {};
    while ( defined( my $word = <FH> ) )    # && ++$wordcount < 1000)
    {
        ++$wordcount;
        $word =~ tr/a-z//cd;

        #print "$word\n";
        my $wordkey = sortword($word);
        my $entry   = $word;                # must be local?
        push @{ $words{$wordkey} }, $entry; #auto-vivifies the array ref?
    }
    close FH;

    #	use Data::Dumper;
    #	print Dumper(\%words);
    my @wordkeys = keys %words;
    print "There are ", scalar @wordkeys,
      " possible letter combinations in $wordcount words\n";
    my @anahist = ();
    for my $wordkey (@wordkeys) {
        ++$anahist[ @{ $words{$wordkey} } ];
        if ( @{ $words{$wordkey} } >= 10 ) {
            printf "%d: %s: ", scalar @{ $words{$wordkey} }, $wordkey;
            print join ",", @{ $words{$wordkey} };
            print "\n";
        }
    }
    for (1..12) {
	printf "There are $anahist[$_] letter combinations that make $_ words\n";
    }
}

&readwords;

#my $pat='s?x';
while (1) {
    print "Enter letters:";
    my $pat = lc <>;
    1 while $pat =~ s/[^\b][\b]//g;
    $pat =~ tr/a-z #?//dc;
    last unless length $pat;
    my $letters = lc $pat;
    $letters =~ tr/a-z//cd;
    my $ucletters = uc $letters;
    my $lcletters = lc $letters;
    my $minwild   = $pat =~ tr/a-zA-Z/?/c;

    for my $wildcount ( $minwild .. $minwild + 1 ) {
        my @results = ();
        my %tried   = ();
        for my $wild ( 'a' x $wildcount .. 'z' x $wildcount ) {
            my $word     = $letters . $wild;
            my $sortword = sortword($word);
            my $sortwild = sortword($wild);

            #print "$word\n";
            push @results, @{ $words{$sortword} }
              if ( defined $words{$sortword} 
              	  and not defined $tried{$sortwild});
            ++$tried{$sortwild};
        }

        print "$wildcount:";
        print join " ",
          map { $_ = uc $_; eval "tr/$ucletters/$lcletters/"; $_ } @results;
        print "\n";
    }

}
