<?
require_once ("common.php");
check_user ();
db_connect ();
html_start ("Main Menu");
?>
<H3>Manage Playlist Templates</H3>

<P><a href="edittemplateproperties.php">Create New Playlist Template</a></P>

<P>
<form name = "select_template" action="edittemplate.php" method="post">
<select name="template_id">
<? display_template_list (-1); ?>
</select>
<input type = "submit" value="Edit Template">
</form>
</P>

<H3>Manage schedule</H3>

<P><a href = "addtimeslot.php">Add Time Slot</a></P>

<form name = "view_schedule" action="viewschedule.php" method="post">
<div>
<label for="start_date">Enter start date :</label>
<input type = text name="start_date" id="start_date">
</div>
<div>
<label for="end_date">Enter end date:</label>
<input type = text name="end_date" id="end_date">
</div>
<input type="submit" value="View Schedule">
</form>

<P><a href = "copyschedule.php">Copy Time Slots</a></P>

<H3>Other Operations</H3>

<P><a href = "updatedb.php">Update Database</a></P>
<?
html_end ();
?>
