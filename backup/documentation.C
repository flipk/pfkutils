
/** \mainpage PFK Backup Database

This is the documentation for the PFK Backup Database (pfkbak).

\section PfkBakOverview Overview

The purpose of pfkbak is to provide a means to quickly generate a backup
of a set of files from one computer to another or from one media to another.

\section PfkBakDefs Definitions

<ul>
<li>  User files / User file tree / User tree

      This term refers to the set of files that the user wishes to
      protect with a backup.

<li>  Database

      This term refers to the backup database file that is produced and
      maintained by the pfkbak tool.

<li>  Backup run / run

      This term refers to one invocation of the pfkbak tool for the purpose
      of updating the database with the latest version of the user files.

<li>  Incremental backup

      This term refers to storing only the changes in user files from one
      run to the next, as opposed to a "full backup" where every user file
      is copied to the backup database regardless of change since last run.

<li>  Backup History

      This term refers to the ability to retrieve older versions of a file
      from previous runs.  For example if a file was modified or deleted,
      and several runs have been done since the modification, it is possible
      to go back several runs and retrieve that previous version of the file.

</ul>

\section PfkBakFeat  Features
The pfkbak tool supports the following features:

<ul>
<li> Single-file database.

     The tool produces, as output, a single data file, which can be
     easily copied to another location or to offline media.  Other
     tools may produce many files, such as a new file or set of files
     for each run, as well as separate "catalog" files, etc.

     On each successive run, the same file is opened and modified, and
     only in the parts which need to change.

     Other file formats cannot be modified in place (e.g. ZIP or TAR).
     The only way to change one file in a ZIP or TAR file is to
     recreate the entire file.

<li> Multiple user trees per database

     The database file can support up to 16 independent virtual backup
     databases within it.  Each backup database is identified with a
     string name, and also remembers the directory path to the user
     tree.  When an incremental backup run is started, pfkbak looks up
     the path to the user tree automatically so the invoker does not
     have to specify it.

     The pfkbak tool can also be invoked once with multiple backup
     names in a single incremental run, in case a single computer has
     several disjoint directories which need to be backed up.  All of
     the backups listed will be updated before the tool exits.

<li> On-the-fly compression.

     As files are written to the backup database, it is compressed
     using <a href="http://www.zlib.net">zlib</a> at the maximum
     compression setting.  (Most backup utilities offer compression,
     so this is nothing new to pfkbak.)

<li> Incremental updates.

     Some backup tools copy every file to the backup on every run.
     Some (better) backup tools only copy files which have changed
     since the last run, but in order to do so, they create new
     additional files in addition to the previous backup files.
     pfkbak stores the updates in-place in the single data file.

<li> Partial file updates.

     Each file is stored in the database as a series of blocks, each
     with its own MD5 hash.  Many file types are only sparsely updated
     in random-access fashion from one backup run to the next, e.g. if
     you add one email to a folder in a 300MB MS Outlook PST file,
     only a few kilobytes of that 300MB file will be modified. Most
     other backup solutions would copy the entire file into the backup
     each time, but pfkbak divides the file into relatively small
     blocks (16kb) and only updates the blocks in the database which
     have been updated.  This saves considerable time in both
     compression effort and in access time to the database, especially
     if the database is accessed over a network.

<li> Full history storage.

     Each time a backup is run, a new run-record is created in the
     database.  Previous run-records are also preserved, and only
     deleted when you choose.  Each run-record is small, only a
     handful of bytes per block in the user files.  If a file is the
     same as in any previous run in the database, the data is not
     duplicated in the database.  If a file only has a few modified
     blocks since the last run, only the modified blocks are written.
     The remainder of the blocks are not updated, and the new
     run-record simply refers to the previous version of those blocks.

<li> Moved file detection.

     If a file is moved from one directory to another in the user tree
     in between two runs, most backup tools will identify this as a
     deletion in the old location and a creation in the new place,
     thus consuming the full amount of space for both runs.  pfkbak
     will recognize this and simply refer to the previous space.  If
     the file was moved to a new place and then modified, pfkbak will
     still recognize the unaltered blocks.

<li> Efficient old run-record deletion.

     When an old run is no longer needed, the run-record is deleted
     from the database.  As the run-record is being deleted, any file
     blocks which are no longer referenced by any run-record are also
     deleted.  Finally, the database file is automatically compacted
     and shrunk to maximize efficiency of storage space.  (Many backup
     tools will increase their space consumption per run without bound,
     requiring the user to periodically scrap the backup set and re-run
     a full backup with an empty database in order to reclaim space for
     the backup.)

</ul>

 */
