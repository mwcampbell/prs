<?
require_once ("common.php");
check_user ();
db_connect ();

/* Delete the specified event */

$query = "delete from playlist_event where template_id = " . $_POST["$template_id"] .
	" and event_number = " . $_POST["event_number"];
db_query ($query);

/* Change event numbers to fill the gap */

$query = "update playlist_event set event_number = event_number-1 where " .
	"template_id = " . $_POST["template_id"] . " and event_number > " . $_POST["event_number"];
db_query ($query);
$redirect_string = "edittemplate.php?template_id=" . $_POST["template_id"];
redirect ($redirect_string);
?>
