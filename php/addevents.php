<?
require_once ("common.php");
check_user ();
db_connect ();

/* Ensure that $template_events_repeat contains something */

if (!$_POST["template_events_repeat"])
	$template_events_repeat = "0";
else $template_events_repeat = $_POST["template_events_repeat"];

/* Connect to the prs database */

if ($_POST["new_template"])
{

	list ($artist_exclude_hours, $artist_exclude_minutes) =
		explode (":", $_POST["template_artist_exclude"], 2);
	$artist_exclude = $artist_exclude_hours*3600+$artist_exclude_minutes*60;
	list ($recording_exclude_hours, $recording_exclude_minutes) =
		explode (":", $_POST["template_recording_exclude"], 2);
	$recording_exclude = $recording_exclude_hours*3600+$recording_exclude_minutes*60;

	$query = "insert into playlist_template (template_name, repeat_events, handle_overlap, artist_exclude, recording_exclude) values ('" . addslashes ($_POST["template_name"]) . "', $template_events_repeat, " . $_POST["template_handle_overlap"] . ", $artist_exclude, $recording_exclude)";
	db_query ($query);
	$template_id = mysql_insert_id ();
} else if ($_SERVER["REQUEST_METHOD"] == "GET")
	$template_id = $_GET["template_id"];
else $template_id = $_POST["template_id"];

// At this point, $template_id has the ID of the template, either from MySQL,
// from a form, or from createvent.php via a redirect (which uses "get").

html_start ("Add Playlist Event");

// Do this to avoid multiple long lines.
$url_query_base = 'template_id=' . $template_id .
	'&amp;event_number=' . $_POST["event_number"] .
	'&amp;insert_event=' . $_POST["insert_event"];

echo "<ul>\n";
echo '<li><a href="randomevent.php?' . $url_query_base . '&amp;event_type=random">Add Random Event</a></li>' . "\n";
echo '<li><a href="randomevent.php?' . $url_query_base . '&amp;event_type=simple_random">Add Simple Random Event</a></li>' . "\n";
echo '<li><a href="urlevent.php?' . $url_query_base . '">Add URL Event</a></li>' . "\n";
echo '<li><a href="pathevent.php?' . $url_query_base . '">Add Path Event</a></li>' . "\n";
echo '<li><a href = "edittemplate.php?template_id=' . $template_id . '">Edit this template</a></li>' . "\n";
echo '<li><a href = "template.php">Back to Playlist Template Administration</a></li>' . "\n";
echo "</ul>\n";

html_end ();
?>
