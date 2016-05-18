<?php

$redis = new Redis();
$redis->connect('127.0.0.1',6379);

if(!$redis->get('key')){
  $redis->sadd('key','12');
  $redis->expire('key',120);
};

$redis->sadd('key','123');
$result = $redis->sMembers();

echo json_encode($result);
