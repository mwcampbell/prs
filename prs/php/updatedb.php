<?
require_once ("common.php");
check_user ();
db_connect ();

html_start ("Update Database");
if (!$audio_root) {
?>
<form name = "update_db" action="updatedb.php" method="post">
<label for="audio_root">Enter the rot directory containing the audio files you wish to add/update:</label>
<input type = "text" name="audio_root" id="audio_root">
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
</select>
<label for="new_category">Or enter a new category in which to place the files:</label>
<input type = "text" name="new_category" id="new_category">
<input type = "submit" value="Start Update">
</form>
<?
}
else {
	$commandline = "/usr/local/bin/prs_db_builder ";
	$commandline .= "-C \"".$station."\" ";
	if ($existing_category)
		$commandline .= "-c \"".$existing_category."\" ";
	else
		$commandline .= "-c \"".$new_category."\" ";;
	$commandline .= " \"".$audio_root."\" >& /dev/null";
	exec ($commandline);
echo "Returned $return\n";
?>
Database update in progress...
<a href = "main.php">Back to main Administration page</a>
<?
}
html_end ();
?>
