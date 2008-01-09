<?
require_once ("common.php");
check_user ();
db_connect ();

html_start ("Update Database");
if (!$_POST["audio_root"]) {
?>
<form name = "update_db" action="updatedb.php" method="post">
<div>
<label for="audio_root">Enter the root directory containing the audio files you wish to add/update:</label>
<input type = "text" name="audio_root" id="audio_root"><br>
<label for="existing_category">Select a category in which to place all files under this directory:</label>
<select name = "existing_category" id="existing_category">
<?
        $res = db_query ("select category_name from category");
        echo "<option value=\"\"></option>";
	while ($row = mysql_fetch_assoc ($res)) {
		$name = $row["category_name"];
		echo "<option value=\"$name\">$name</option>";
	}
        mysql_free_result ($res);
?>
</select><br>
<label for="new_category">Or enter a new category in which to place the files:</label>
<input type = "text" name="new_category" id="new_category"><br>
<input type = "submit" value="Start Update">
</div>
</form>
<?
}
else {
	$commandline = "/usr/local/bin/prs_db_builder ";
	$commandline .= "-C \"" . $_SESSION["station_config"] . "\" ";
	if ($_POST["existing_category"])
		$commandline .= "-c \"" . $_POST["existing_category"] . "\" ";
	else
		$commandline .= "-c \"" . $_POST["new_category"] . "\" ";;
	$commandline .= " \"" . $_POST["audio_root"] . "\" >& /dev/null";
	exec ($commandline);
?>
<p>Database update in progress...</p>
<div><a href = "main.php">Back to main menu</a></div>
<?
}
html_end ();
?>
