<?
require_once ("common.php");
check_user ();
html_start ("Main Menu");
?>
<H3>Manage Playlist Templates</H3>

<a href="newtemplate.php">Create New Playlist Template</a>

<form name = "select_template" action="edittemplate.php" method="post">
<select name="template_id">
<? display_template_list (-1); ?>
</select>
<input type = "submit" value="Edit Template">
</form>

<H3>Manage schedule</H3>

<ul>
<li><a href = "addtimeslot.php">Add Time Slot</a></li>
<li><a href = "viewschedule.php">View/edit Schedule</a></li>
<li><a href = "copyschedule.php">Copy Time Slots</a></li>
</ul>

<a href = "updatedb.php">Update Database</a>
<?
html_end ();
?>
