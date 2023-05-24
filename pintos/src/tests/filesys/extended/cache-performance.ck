use strict;
use warnings;
use tests::tests;

my $expected_output = <<'EOF';
(cache-performance) begin
(cache-performance) create "a" should not fail.
(cache-performance) open "a" returned fd which should be greater than 2.
(cache-performance) write should write all the data to file.
(cache-performance) File creation completed.
(cache-performance) First run cache hit rate calculated.
(cache-performance) Second run cumulative cache hit rate calculated.
(cache-performance) Hit rate should grow.
(cache-performance) Removed "a".
(cache-performance) end
cache-performance: exit(0)
EOF

check_expected (IGNORE_USER_FAULTS => 1, [$expected_output]);

pass;
