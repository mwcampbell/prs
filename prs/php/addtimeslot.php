<?
require_once ("common.php");
check_user ();
db_connect ();

if ($start_date && $length) {
	$start_date = strtotime ($start_date);
	if ($start_date == -1) {
		html_error ("Start or end date out of range.\n");
		exit ();
	}
	
	/* Convert length of seconds */

	list ($length_hours, $length_minutes, $length_seconds) = explode (":", $length, 3);
	$length = $length_hours*3600+$length_minutes*60+$length_seconds;

	$daylight = date ("I", $start_date);
	$query = "insert into schedule (start_time, length, repetition,
        daylight, template_id, fallback_id, end_prefade) values
        ($start_date, $length, $repetition,
        $daylight, $template_id, $fallback_id, $end_prefade)";
	db_query ($query);
	html_start ("Schedule updated");
	echo "<a href = \"schedule.php\">Back to Schedule Administration</a>\n";
	$schedule_updated = 1;
}
else {
        html_start ("Update Schedule");
        $schedule_updated = 0;
}

?>
<form name="add_time_slot" action="addtimeslot.php" method="post">
<div>
<label for="start_date">Enter start date:</label>
<input type="text" name="start_date" id="start_date">
</div>
<div>
<label for="length">Enter length (hh:mm:ss):</label>
<input type="text" name="length" id="length">
</div>
<div>
<label for="repetition">Repetition:</label>
<select name="repetition" id="repetition" value="0">
<option value="0">One Time Only</option>
<option value="3600">Hourly</option>
<option value="86400">Daily</option>
<option value="604800">Weekly</option>
</select>
</div>
<div>
<label for="template_id">Select playlist template</label>
<select name="template_id" id="template_id" value="0">
<? display_template_list () ?>
</select>
</div>
<div>
<label for="fallback_id">Select fallback playlist template</label>
<select name="fallback_id" id="fallback_id" value="-1">
<? display_template_list () ?>
</select>
</div>
<div>
<input type="text" name="end_prefade" id="end_prefade">
</div>
<div>
<input type="submit" value="Update Schedule">
</form>
<?
html_end ();
exit ();
?>