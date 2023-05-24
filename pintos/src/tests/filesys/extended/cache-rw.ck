use strict;
use warnings;
use tests::tests;

my $expected_output = <<'EOF';
(cache-rw) begin
(cache-rw) create "a" should not fail.
(cache-rw) File creation completed.
(cache-rw) open "a" returned fd which should be greater than 2.
(cache-rw) write completed.
(cache-rw) Block write count should be less than 134.
(cache-rw) Block read count should be less than acceptable error 6.
(cache-rw) Removed "a".
(cache-rw) end
cache-rw: exit(0)
EOF

check_expected (IGNORE_USER_FAULTS => 1, [$expected_output]);

pass;
