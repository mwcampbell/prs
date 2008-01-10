<?
require_once ("common.php");
check_user ();
db_connect ();

if ($_SERVER["REQUEST_METHOD"] == "GET")
	$template_id = $_GET["template_id"];
else $template_id = $_POST["template_id"];

html_start ("Add Playlist Event");

// Do this to avoid multiple long lines.
$url_query_base = 'template_id=' . $template_id .
	'&amp;event_number=' . $_POST["event_number"] .
	'&amp;insert_event=' . $_POST["insert_event"] .
	'&amp;event_type=';

echo "<ul>\n";
echo '<li><a href="editevent.php?' . $url_query_base . 'random">Add Random Event</a></li>' . "\n";
echo '<li><a href="editevent.php?' . $url_query_base . 'simple_random">Add Simple Random Event</a></li>' . "\n";
echo '<li><a href="editevent.php?' . $url_query_base . 'url">Add URL Event</a></li>' . "\n";
echo '<li><a href="editevent.php?' . $url_query_base . 'path">Add Path Event</a></li>' . "\n";
echo '<li><a href = "edittemplate.php?template_id=' . $template_id . '">Edit this template</a></li>' . "\n";
echo '<li><a href = "main.php">Back to Main Menu</a></li>' . "\n";
echo "</ul>\n";

html_end ();
?>
