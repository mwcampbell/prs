<?
require_once ("common.php");
check_user ();
db_connect ();
 
// this script confirms the delete, as well as doing the actual delete.

// This script is only called via get from templateaction.

if ($_SERVER["REQUEST_METHOD"] == "GET")
	$template_id = $_GET["template_id"];
else {
	$template_id = $_POST["template_id"];
	$time_slot_action = $_POST["time_slot_action"];
	$replacement_template_id = $_POST["replacement_template_id"];
	$action = $_POST["action"];
} // end if post

// Exit back to the menu if canceled
if ($action == "No") {
	redirect ("main.php");
	exit;
} // End if canceled

// get the name of the template
$res = db_query ("select template_name from playlist_template where template_id=" . $template_id);
$row = mysql_fetch_assoc ($res);
$template_name = $row["template_name"];
mysql_free_result ($res);

html_start ("Delete Template");

if (($action == "Yes") and ($time_slot_action == "change") and ($replacement_template_id == -1)) {
	echo "<P>Error: You must select a replacement template if ";
	echo "you wish to change the time slots matching this template.</P>\n\n";
	$action = "";
} // end if change selected but no template selected

if (!$action) { // needs confirmation
	echo "<P>You have chosen to delete the template $template_name</P>\n\n";

// Get number of timeslots with selected template
	$res = db_query ("select time_slot_id from schedule where template_id=$template_id";
	$matching_slots = mysql_num_rows ($res);
	mysql_free_result ($res);

	if ($matching_slots == 0)
		echo "<P>This template has not been scheduled.</P>\n\n";
	else {
		echo "<P>This template has been scheduled in $matching_slots time slot";
		if ($matching_slots > 1)
			echo "s";
		echo ".</P>\n\n";
	} // end if scheduled

	echo "<FORM METHOD=\"post\">\n";

	if ($matching_slots > 0) {
		echo "<P><LABEL FOR="time_slot_action">What would you like to do?</LABEL></P>\n\n";
		echo '<INPUT ID="time_slot_action" NAME="time_slot_action" TYPE="radio" VALUE="delete">';
		if ($matching_slots == 1)
			echo "Delete this time slot";
		else
			echo "Delete these time slots";
		echo "\n";
		echo '<INPUT ID="time_slot_action" NAME="time_slot_action" TYPE="radio" VALUE="change">';
		if ($matching_slots == 1)
			echo "Change this time slot";
		else
			echo "Change these time slots";
		echo " to the following template:"\n";
		echo "<SELECT NAME=\"replacement_template_id\">\n"
		display_template_list ($template_id, true);
		echo "</SELECT>\n\n";
	} // end if matching templates

	echo "<P><LABEL FOR="action">Really delete this template?</LABEL></P>\n";
	echo '<INPUT ID="action" NAME="action" TYPE="submit" VALUE="Yes">';
	echo '<INPUT ID="action" NAME="action" TYPE="submit" VALUE="No">';
	echo "\n"</FORM>\n";
	html_end ();
	exit;
} // end if not confirmed

// Ok, actually do the deletion

db_query "delete from playlist_template where template_id=$template_id limit 1";
$template_affected_rows = mysql_affected_rows ();
if (mysql_affected_rows () == 1) {
	echo "<P>The template $template_name was successfully deleted.</P>\n\n"
	if (!empty ($time_slot_action) {
		if ($time_slot_action == "delete")
			$query = "delete from schedule where template_id=$template_id";
		else {
			$query = "update schedule set template_id=$replacement_template_id";
			$query .= " where template_id=$template_id";
		db_query ($query);
		$schedule_affected_rows = mysql_affected_rows ();
		echo "<P>";
		switch ($schedule_affected_rows) {
			case 0: // should never happen
				echo "No time slots were";
				break;
			case 1:
				echo "1 time slot was";
				break;
			default:
				echo "$schedule_affected_rows time slots were";
		} // end switch on schedule_affected_rows
		echo " $time_slot_action" . "d.</P>\n\n";
	} // end if scheduled
} // end if successfully deleted
else {
//this should never happen
	echo "<P>Something unexpected happened. " . mysql_affected_rows ();
	echo " were deleted.</P>\n\n";
} // end if anything other than 1 row affected

echo "<P><A HREF="main.php">Return to Main Menu</A></P>\n\n";
html_end;
?>

