#!/usr/bin/env php
<?php
header("Content-Type: text/html");

echo "<html><head><title>PHP CGI Info</title></head>\n";
echo "<body>\n";
echo "<h1>PHP CGI Information</h1>\n";
echo "<p>This is a PHP script running through CGI.</p>\n";

echo "<h2>PHP Version:</h2>\n";
echo "<p>" . phpversion() . "</p>\n";

echo "<h2>Environment Variables:</h2>\n";
echo "<table border='1'>\n";
echo "<tr><th>Name</th><th>Value</th></tr>\n";
foreach($_SERVER as $key => $value) {
    $display_value = is_array($value) ? print_r($value, true) : $value;
    echo "<tr><td>$key</td><td>" . htmlspecialchars($display_value) . "</td></tr>\n";
}
echo "</table>\n";

echo "</body></html>\n";
?>