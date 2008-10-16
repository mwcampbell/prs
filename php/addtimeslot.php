<?
require_once ("common.php");
check_user ();
db_connect ();

if ($_POST["start_date"] && $_POST["end_date"]) {
	$start_date = strtotime ($_POST["start_date"]);
	if ($start_date == -1) {
		html_error ("Start date out of range.\n");
		exit ();
	}
	$end_date = strtotime ($_POST["end_date"]);
	if ($end_date == -1) {
		html_error ("End date out of range.\n");
		exit ();
	}
	$length = $end_date-$start_date;

// The following returns 1 for daylight savings, 0 otherwise.
	$daylight = date ("I", $start_date);

	$query = "insert into schedule (start_time, length, repetition,
        daylight, template_id, fallback_id, end_prefade) values
        ($start_date, $length, " . $_POST["repetition"] . ", " .
        $daylight . ", " . $_POST["template_id"] . ", " .
	$_POST["fallback_id"] . ", " . $_POST["end_prefade"] . ")";
	db_query ($query);
	html_start ("Add Time slot");
echo "<P>Time slot added successfully.</P>\n";
	echo "<a href = \"main.php\">Back to Main Menu</a>\n";
	$schedule_updated = 1;
}
else {
        html_start ("Add Time Slot");
        $schedule_updated = 0;
}

?>
<form name="add_time_slot" action="addtimeslot.php" method="post">
<div>
<label for="start_date">Enter start date:</label>
<input type="text" name="start_date" id="start_date">
</div>
<div>
<label for="end_date">Enter end date:</label>
<input type="text" name="end_date" id="end_date">
</div>
<div>
<label for="repetition">Repetition:</label>
<?
display_repetition_list (0);
?>
</div>
<div>
<label for="template_id">Select playlist template</label>
<select name="template_id" id="template_id" value="0">
<? display_template_list (-1) ?>
</select>
</div>
<div>
<label for="fallback_id">Select fallback playlist template</label>
<select name="fallback_id" id="fallback_id" value="-1">
<? display_template_list (-1) ?>
</select>
</div>
<div>
<label for="end_prefade">Fade time at end of template:</label>
<input type="text" name="end_prefade" id="end_prefade">
</div>
<div>
<input type="submit" value="Update Schedule">
</form>
<?
html_end ();
exit ();
?>
