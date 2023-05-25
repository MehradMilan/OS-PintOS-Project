use strict;
use warnings;
use tests::tests;

my $expected_output = <<'EOF';
(cache-perform) begin
(cache-perform) create "a" should not fail.
(cache-perform) open "a" returned fd which should be greater than 2.
(cache-perform) write should write all the data to file.
(cache-perform) File creation completed.
(cache-perform) First run cache hit rate calculated.
(cache-perform) Second run cumulative cache hit rate calculated.
(cache-perform) Hit rate should grow.
(cache-perform) Removed "a".
(cache-perform) end
cache-perform: exit(0)
EOF

check_expected (IGNORE_USER_FAULTS => 1, [$expected_output]);

pass;