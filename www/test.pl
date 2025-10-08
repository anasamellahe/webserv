#!/usr/bin/perl
use strict;
use warnings;

# Print CGI headers
print "Content-Type: text/html\r\n\r\n";

# Print HTML content
print <<'EOF';
<!DOCTYPE html>
<html>
<head>
    <title>Perl CGI Test</title>
</head>
<body>
    <h1>Perl CGI Test Script</h1>
    <p>This is a simple Perl CGI script for testing.</p>
    
    <h2>Request Information:</h2>
EOF

# Print environment variables
print "<p>Method: " . ($ENV{REQUEST_METHOD} || 'Unknown') . "</p>\n";
print "<p>Query String: " . ($ENV{QUERY_STRING} || 'None') . "</p>\n";
print "<p>Remote Address: " . ($ENV{REMOTE_ADDR} || 'Unknown') . "</p>\n";
print "<p>Script Name: " . ($ENV{SCRIPT_NAME} || 'Unknown') . "</p>\n";
print "<p>Server Name: " . ($ENV{SERVER_NAME} || 'Unknown') . "</p>\n";

print <<'EOF';
    
    <h2>Form Example:</h2>
    <form method="post" action="">
        Name: <input type="text" name="name"><br><br>
        Message: <textarea name="message"></textarea><br><br>
        <input type="submit" value="Submit">
    </form>
    
    <h2>All Environment Variables:</h2>
    <ul>
EOF

# Print all environment variables
foreach my $key (sort keys %ENV) {
    my $value = $ENV{$key} || '';
    # HTML escape the values
    $value =~ s/&/&amp;/g;
    $value =~ s/</&lt;/g;
    $value =~ s/>/&gt;/g;
    $value =~ s/"/&quot;/g;
    print "        <li><strong>$key:</strong> $value</li>\n";
}

print <<'EOF';
    </ul>
</body>
</html>
EOF