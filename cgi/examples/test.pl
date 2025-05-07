#!/usr/bin/env perl
use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

print $cgi->header('text/html');
print $cgi->start_html('Perl CGI Test');
print $cgi->h1('Perl CGI Test Script');
print $cgi->p('This is a simple Perl CGI script for testing.');

print $cgi->h2('Request Information:');
print $cgi->p('Method: ' . $ENV{REQUEST_METHOD});
print $cgi->p('Query String: ' . $ENV{QUERY_STRING});
print $cgi->p('Remote Address: ' . $ENV{REMOTE_ADDR});

print $cgi->h2('Form Example:');
print $cgi->start_form;
print 'Name: ', $cgi->textfield('name'), $cgi->br;
print 'Message: ', $cgi->textarea('message'), $cgi->br;
print $cgi->submit('Submit');
print $cgi->end_form;

if ($cgi->param()) {
    print $cgi->h2('Form Data Received:');
    foreach my $param ($cgi->param()) {
        print $cgi->p("$param: " . $cgi->param($param));
    }
}

print $cgi->end_html;