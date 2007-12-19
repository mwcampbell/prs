<?
require_once ("common.php");
check_user ();
html_start ("Main Menu");
?>
<H3>Manage Playlist Templates</H3>

<ul>
<li><a href="newtemplate.php">Create New Playlist Template</a></li>
<li><a href = "edittemplate.php">Edit an Existing Playlist Template</a></li>
</ul>

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
