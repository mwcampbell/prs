<?
require_once ("common.php");
check_user ();
db_connect ();

if ($start_date && $end_date) {
	$start_date = strtotime ($start_date);
	$end_date = strtotime ($end_date);
	if ($start_date == -1 || $end_date == -1) {
		html_error ("Start or end date out of range.\n");
		exit ();
	}
	$query = "select * from schedule where start_time <= $start_date and end_time > $start_date";
	$res = db_query ($query);
	if (mysql_numrows ($res) > 0) {
		html_error ("Error-- time slot conflicts with existing time slot");
		html_end ();
		exit ();
	}

	$query = "insert into schedule (start_time, end_time, template_id, fallback_id, end_prefade) values ($start_date, $end_date, $template_id, $fallback_id, $end_prefade)";
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
<label for="end_time">Enter end date:</label>
<input type="text" name="end_date" id="end_date">
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