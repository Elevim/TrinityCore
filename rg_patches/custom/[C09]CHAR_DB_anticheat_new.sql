-- ----------------------------------
-- AntiCheat-Log-Schema
-- -----------------------------------

DROP TABLE IF EXISTS `anticheat_log`;
 
CREATE TABLE `anticheat_log` (
  `charname` varchar(32) NOT NULL,
  `checktype` mediumint(8) UNSIGNED NOT NULL,
  `map` smallint(5) NOT NULL,
  `zone` smallint(5) NOT NULL,
  `debugmsg` text,
  `alarm_time` datetime NOT NULL,
  `guid` int(11) UNSIGNED NOT NULL,
  `action1` mediumint(8) NOT NULL DEFAULT '0',
  `action2` mediumint(8) NOT NULL DEFAULT '0',
  `lastSpell` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`checktype`,`alarm_time`,`guid`),
  KEY `idx_Player` (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Anticheat log table';
