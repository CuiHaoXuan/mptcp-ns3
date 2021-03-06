#!/usr/bin/perl -w
# test_forward_exact_port

use strict;
use OF::Includes;

sub forward_port {

	forward_simple(@_, 'port');
}

sub my_test {

	my ( $sock, $options_ref ) = @_;

	enable_flow_expirations( $ofp, $sock );

	for_all_port_pairs( $ofp, $sock, $options_ref, \&forward_port, 0x0);
}

run_black_box_test( \&my_test, \@ARGV );
