--TEST--
PostgreSQL drop db
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
// drop test table 

include('config.inc');

$db = pg_connect($conn_str);
pg_query($db, "DROP TABLE ".$table_name);

echo "OK";

?>
--EXPECT--
OK
