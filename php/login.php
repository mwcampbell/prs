<?
require_once ("common.php");

function
startElement ($parser, $name, $attrs) {

	if ($name == "DB") {
// We can add to the session because it was started in common.php.
// We add these now so that db_connect() can use it in all circumstances.
		$_SESSION["DB_USER"] = $attrs["USER"];
		$_SESSION["DB_HOST"] = $attrs["HOST"];
		$_SESSION["DB_PASSWORD"] = $attrs["PASSWORD"];
		$_SESSION["DB_NAME"] = $attrs["NAME"];
	}
}

function
endElement ($parser, $name) {
}

function
parse_config_file ($filename) {
	$parser = xml_parser_create ();
	xml_set_element_handler ($parser, "startElement", "endElement");
	
	if (!$fp = fopen ($filename, "r")) {
		html_error ("Configuration file " . $filename . " not found.");
		exit ();
	}
	while ($data = fread ($fp, 4096)) {
		if (!xml_parse ($parser, $data, feof($fp)))
			html_error ("XML error parsing " . $filename . ": " .
				xml_error_string(xml_get_error_code($parser)) .
				" on line " . xml_get_current_line_number($parser));
	} // end while
} // end parse_config_file()

parse_config_file ($_POST["station_config"]);

db_connect ();

if (!$_POST["username"] && !$_SESSION["user_username"])
	html_error ("Login error.");
else if (!$_SESSION["user_username"])
{
	$query = "select password, type from prs_user where user_name = '" . $_POST["username"] . "'";
	$res = db_query ($query);
	if ($row = mysql_fetch_assoc ($res))
	{
		$real_password = $row["password"];
		$real_type = $row["type"];
	}
	mysql_free_result ($res);
	if (!$real_password)
	{
		html_error ("there is no user by that name.");
	}
	else if ($real_password != $_POST["password"])
	{
		html_error ("Wrong password.");
	}
	else
	{
// We can assign to $_SESSION because session_start() was called in common.php
		$_SESSION["user_username"] = $_POST["username"];
		$_SESSION["user_password"] = $real_password;
		$_SESSION["user_type"] = $real_type;

        	foreach ($STATIONS as $name => $config)
        {
                	if ($config == $_POST["station_config"])
				$station_name = $name;
        }

		$_SESSION["station_name"] = $station_name;
		$_SESSION["station_config"] = $_POST["station_config"];

// DB session vars were set in startElement();
	}
}
redirect ("main.php");
?>
