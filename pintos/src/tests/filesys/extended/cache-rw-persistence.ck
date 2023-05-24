use strict;
use warnings;
use tests::tests;
use tests::random;

sub generate_and_check {
    my $size = shift;
    my $random_bytes = tests::random::random_bytes($size);
    tests::tests::check_archive({});
    return $random_bytes;
}

my ($a) = generate_and_check(8143);
my ($b) = generate_and_check(8143);
pass;
