
/** \mainpage PFK Backup Database

This is the documentation for the PFK Backup Database (pfkbak).

\section PfkBakOverview Overview

The purpose of pfkbak is to provide a means to quickly generate a
backup of a set of files from one computer to another or from one
media to another.

This is a tool for archiving a set of files or a directory tree into
an archive file, much like 'tar' does.  The difference is that this
tool can selectively replace/update individual files in the archive in
a space- and time-efficient manner.  In a 'tar' file, if you want to
update one file in the archive, you must re-create the entire archive.
even 'zip' files have this problem.

Another problem is during extraction, if you want only one file or a
directory of files extracted, you must wait while the entire archive
is read linearly from start to finish.  This is true with TAR files,
although it is not true with ZIP and some other formats.  With this
tool, you can extract a single file or directory instantly because it
can seek directly to the part of the archive which stores that file.

This version also includes compression, for example using libz to
block-compress each file.

This version also includes incremental backups with full history, so
that the contents of a file from several backups previously can be
recovered.  It attempts to be extremely efficient with the file in
order to minimize the amount of time required to 'freshen' the backup.
The idea being that backups may be freshened daily or perhaps even
hourly.  If a file is modified or even deleted, and then a backup is
run, the file's previous contents are kept in the backup and can be
retrieved by specfifying a previous 'generation number'.

Each time that a backup is run, the generation number is incremented
by 1.  Every bit of data in the backup is tagged with a generation
number of when it was added to the backup.  The backup also maintains
a list of all generations still present in the file (as well as the
date when that particular backup session was run).  The user may
retrieve a list of all generations in the file, and selectively delete
them.  If a file in the backup has a version of data associated with
that generation, the data may be freed from the backup (unless of
course that particular data is associated with additional
generations).


<ul>
<li>  \ref Definitions
<li>  \ref Features
<li>  \ref CommandLine
<li>  \ref DatabaseEntries
<li>  \ref Algorithms
</ul>

*/

/** \page Definitions Definitions

\section PfkBakDefs Definitions

<ul>
<li>  <b> User files / User file tree / User tree </b>
      <br> This term refers to the set of files that the user wishes to
      protect with a backup.
<li>  <b> Database </b>
      <br> This term refers to the backup database file that is produced and
      maintained by the pfkbak tool.
<li>  <b> Backup run / run </b>
      <br> This term refers to one invocation of the pfkbak tool for the purpose
      of updating the database with the latest version of the user files.
<li>  <b> Incremental backup </b>
      <br> This term refers to storing only the changes in user files from one
      run to the next, as opposed to a full backup.
<li>  <b> Full backup </b>
      <br> Every user file is copied to the backup database regardless of
      change since last run.
<li>  <b> Backup History </b>
      <br> This term refers to the ability to retrieve older versions of a file
      from previous runs.  For example if a file was modified or deleted,
      and several runs have been done since the modification, it is possible
      to go back several runs and retrieve that previous version of the file.
</ul>

*/

/** \page Features Features

The pfkbak tool supports the following features:

<ul>
<li> <b> Single-file database. </b>
     <br> The tool produces, as output, a single data file, which can be
     easily copied to another location or to offline media.  Other
     tools may produce many files, such as a new file or set of files
     for each run, as well as separate "catalog" files, etc.
     <br> On each successive run, the same file is opened and modified, and
     only in the parts which need to change.
     <br> Other file formats cannot be modified in place (e.g. ZIP or TAR).
     The only way to change one file in a ZIP or TAR file is to
     recreate the entire file.
<li> <b> Multiple user trees per database </b>
     <br> The database file can support up to 16 independent virtual backup
     databases within it.  Each backup database is identified with a
     string name, and also remembers the directory path to the user
     tree.  When an incremental backup run is started, pfkbak looks up
     the path to the user tree automatically so the invoker does not
     have to specify it.
     <br> The pfkbak tool can also be invoked once with multiple backup
     names in a single incremental run, in case a single computer has
     several disjoint directories which need to be backed up.  All of
     the backups listed will be updated before the tool exits.
<li> <b> On-the-fly compression. </b>
     <br> As files are written to the backup database, it is compressed
     using <a href="http://www.zlib.net">zlib</a> at the maximum
     compression setting.  (Most backup utilities offer compression,
     so this is nothing new to pfkbak.)
<li> <b> Incremental updates. </b>
     <br> Some backup tools copy every file to the backup on every run.
     Some (better) backup tools only copy files which have changed
     since the last run, but in order to do so, they create new
     additional files in addition to the previous backup files.
     pfkbak stores the updates in-place in the single data file.
<li> <b> Partial file updates. </b>
     <br> Each file is stored in the database as a series of blocks, each
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
<li> <b> Full history storage. </b>
     <br> Each time a backup is run, a new run-record is created in the
     database.  Previous run-records are also preserved, and only
     deleted when you choose.  Each run-record is small, only a
     handful of bytes per block in the user files.  If a file is the
     same as in any previous run in the database, the data is not
     duplicated in the database.  If a file only has a few modified
     blocks since the last run, only the modified blocks are written.
     The remainder of the blocks are not updated, and the new
     run-record simply refers to the previous version of those blocks.
<li> <b> Moved file detection. </b>
     <br> If a file is moved from one directory to another in the user tree
     in between two runs, most backup tools will identify this as a
     deletion in the old location and a creation in the new place,
     thus consuming the full amount of space for both runs.  pfkbak
     will recognize this and simply refer to the previous space.  If
     the file was moved to a new place and then modified, pfkbak will
     still recognize the unaltered blocks.
     \note  This feature is not yet implemented.
<li> <b> Efficient old run-record deletion. </b>
     <br> When an old run is no longer needed, the run-record is deleted
     from the database.  As the run-record is being deleted, any file
     blocks which are no longer referenced by any run-record are also
     deleted.  Finally, the database file is automatically compacted
     and shrunk to maximize efficiency of storage space.  (Many backup
     tools will increase their space consumption per run without
     bound, requiring the user to periodically scrap the backup set
     and re-run a full backup with an empty database in order to
     reclaim space for the backup.)
</ul>

 */


/** \page CommandLine Command Line Forms

<ul>
<li> <b> pfkbak -C[Vv] BACKUP-FILE </b>
   <br> This creates a new (empty) database file with no backups present.
<li> <b> pfkbak -L[Vv] BACKUP-FILE [BACKUP-NAME] </b>
   <br> This displays information about the backups in a database, or, if a
   backup name is specified, about that specific backup.  For each
   backup, it displays the backup name, comment, and a list of all
   existing generations in that backup, as well as some information
   about each generation.
\todo implement the [BACKUP-NAME] argument to this command.

<li> <b> pfkbak -c[Vv] BACKUP-FILE BACKUP-NAME DIR COMMENT </b>
   <br> This creates a new backup in a database. Specify the name of the
   backup, the root directory of the backup, and a comment string
   (which requires shell-quoting and shell-escaping if it is more than
   one word or contains special characters).
   <br> Only the information about the backup is added to the database.  A
   backup run is \em not performed.  You must perform an update to add
   the data for the first run.
<li> <b> pfkbak -D[Vv] BACKUP-FILE BACKUP-NAME [-confirm] </b>
   <br> This deletes all information about a backup from the database.
   This requires user confirmation (unless the -confirm option is
   present).  It deletes the backup info, all generations, and all
   file contents.
\todo implement the -confirm option to this command.

<li> <b> pfkbak -u[Vv] BACKUP-FILE BACKUP-NAME </b>
   <br> This 'updates' a backup to the latest version of all files.  It
   creates a new 'generation' in the backup with the current time
   stamp.  Note that when a new backup is created, this command must
   be run in order to add the first set of data to the backup.
   <br> The root path of the backup is stored in the database, in the
   backup information, so the directory is not specified on the
   command line.
<li> <b> pfkbak -d[Vv] BACKUP-FILE BACKUP-NAME GenN X-Y -Z [...] </b>
   <br> This deletes old generations, presumably to allow the construction
   of a 'tiered' backup approach.  There are multiple ways to specify
   generations for deletion.  A generation number can be provided by
   itself, which deletes only that generation.  A range (X through Y)
   can be specified, where all generation numbers in between X and Y
   (inclusive) will be deleted.  Also an open-ended range (all
   generations thru Z) can be specified which deletes every generation
   number less than or equal to Z.
   <br> The command line may contain any number of generation specifiers.
<li> <b> pfkbak -l[Vv] BACKUP-FILE BACKUP-NAME GenN </b>
   <br> This lists all the files present in a generation, and some information
   about each file.
<li> <b> pfkbak -e[Vv] BACKUP-FILE BACKUP-NAME GenN [FILE...] </b>
   <br> This extracts files from a specified generation.  If a limited set
   of files are desired, they may be listed individually on the
   command line.  The full path of the file name must be specified,
   essentially matching the strings found in the -l output.
   <br> If no filenames are specified on the command line, the entire
   generation is extracted.
   <br> The extraction is done into whatever the current working directory
   of the tool is at the time the tool is invoked.  The extraction is
   \em not done into the root directory of the backup itself.
<li> <b> pfkbak -E[Vv] BACKUP-FILE BACKUP-NAME GenN LIST-FILE </b>
   <br> This also extracts files from a generation.  However the list of
   files to extract are read from a LIST-FILE.  This file is a text
   file containing one file per line.  The file may terminate each
   line with either a line-feed (unix format), a carriage-return plus
   line feed (msdos/win format) or a carriage-return (mac format).
</ul>

 */
/** \page DatabaseEntries Database Entries

<img src="backup-database-design.gif">

The following data types are used in the database file:

<ul>
<li>  <b> backup number </b>
      <br>  This is a 32-bit random number.  It identifies each backup,
      and is guaranteed to be unique when it is generated.
<li>  <b> file number </b>
      <br>  This is a 32-bit value which counts from 0 to 
      PfkBakupInfoData::file_count-1.  It uniquely identifies a file,
      whether that file exists in a particular generation or not.
      File numbers are never reused.  If all generations which contains
      a file are freed, no database entries will exist which use this
      file number, but the number itself will not be freed or reused.
<li>  <b> file piece </b>
      <br>  This is a 32 kilobyte unit of data from a file.
<li>  <b> piece number </b>
      <br>  This is a 32-bit number which describes a file piece.  A
      piece number may be translated into a file offset by multiplying
      by 32k.
<li>  <b> generation number </b>
      <br>  This is a 32-bit number which starts at 1 and increases 
      each time a new run is executed.  The next generation number is
      stored in PfkBackupInfoData.
</ul>

The following entries exist in the database file:

<ul>
<li> <b> PfkBackupDbInfo </b>
   <br> There exists exactly one of these in the entire database.
   It is only updated when a backup set is created or deleted, or when
   a new tool version upgrades the data structures in the file.
   <ul>
   <li> Key prefix: 1
   <li> Key: string info_key
      <ul>
      <li> this is a constant string (the data element is a singleton).
           the key's purpose is to ensure the tool is looking at a database
           created by this tool.
      </ul>
   <li> Data:
      <ul>
      <li> uint32 tool_version
           <br> This field is used to prevent access by the wrong version of
           the tool, in case the data structure layouts have changed 
           between versions.
      <li> uint32 backups[]
           <br> This array lists all the valid backups currently in the file.
           When a new backup is created, the list is grown by one and the
           new entry added at the end.  When a backup is deleted, the last
           entry in the array is pulled down into the empty slot created.
           Each backup number is used in the key of a BackupInfo to locate
           information about that particular backup.
      </ul>
   </ul>
<li> <b> PfkBackupInfo </b>
   <br> There exists one of these data structures for each backup in the 
   database.  It is modified whenever a generation is created or deleted.
   <ul>
   <li> Key prefix: 2
   <li> Key:  uint32 backup_number
   <li> Data: 
      <ul>
      <li> string root_dir
           <br> This is the root directory of this backup.  This is kept
           here so that the user does not have to specify the root directory
           each time an 'update' is performed. 
      <li> string name
           <br> This is a text name for the backup.  The best format for a 
           backup name is string which can be represented as a single
           command line argument in a unix shell (i.e. no whitespaces or
           special characters).
      <li> string comment
           <br> This is a text field describing the backup.  It is populated
           from the command line when a backup is created, and displayed
           when the contents of a database file are requested.  The contents
           are not used for any other purpose.
      <li> uint32 file_count
           <br> This is the count of the number of files found in this
           backup.  The values 0 thru this number minus one are used in
           the key to the following date type.
      <li> uint32 next_generation_number
           <br> This field is used when creating a new generation, and
           incremented each time.
      <li> uint32 generations[]
           <br> This field lists all the generations currently
           existing in the backup.  When a new generation is created,
           the list is grown by one and the new entry added at the
           end.  When a generation is deleted, the last entry is
           pulled down into the hole created.  The generation number
           is used in the key of a GenerationInfo. For each generation,
           the date and time of creation is stored as well as the generation
           number.
      </ul>
   </ul>
<li> <b> PfkBackupFileInfo </b>
   <br> There exists one of these for each file in a backup.
   <ul>
   <li> Key prefix: 3
   <li> Key: uint32 backup_number, uint32 file_number
   <li> Data:
      <ul>
      <li> string file_path
         <br> This is the full path to the file, relative to the
         root directory of this backup.
      <li> uint64 size
         <br> This is the size of this file in bytes, at the time of
         the most recent backup run.
      <li> uint32 mtime
         <br> This is the time (in unix time_t format) of the last
         modification time of this file.
      <li> uint32 generations[]
         <br> This lists all the generations that this file name is
         a part of.  This is used only to determine when it is okay
         to free up the file_number (when the last generation referencing
         a file has been freed).
      </ul>
   </ul>
<li> <b> PfkBackupFilePieceInfo </b>
   <br> 
   <ul>
   <li> Key prefix: 4
   <li> Key: uint32 backup_number, uint32 file_number, uint32 piece_number
   <li> Data: 
      <ul>
      <li> array PfkBackupVersions:
         <ul>
         <li> uint32 gen_number
              <br> The generation number of this version of this piece.
         <li> uchar md5hash[16]
              <br> The md5 hash of this version of this piece.  Use this
              value in the key of the next data type to locate the data
              for this version.
         </ul>
      </ul>
   </ul>
<li> <b> PfkBackupFilePieceData </b>
   <br> 
   <ul>
   <li> Key prefix: 5
   <li> Key: uint32 backup_number, uint32 file_number,
             uint32 piece_number, uchar md5hash[16]
   <li> Data:
      <ul>
      <li> uint32 refcount
           <br> This is a count of the number of generations which refer
           to the data in this piece version.  If this ever is decreased
           to zero, the data must be deleted from the file because no 
           version information references this data any longer.
      <li> uint16 compressed_size
           <br> This is the compressed size of the libz-compressed data.
           This is required because an exact value cannot be extracted
           from the FileBlock API (the FileBlock number is always rounted
           up to the nearest 32-byte boundary).
      <li> uint16 uncompressed_size
           <br> This is the uncompressed size of the libz-compressed data.
           This is always exactly 32kb except in the last piece of a file.
      <li> FB_AUID_t data_fbn
           <br> This is an AUID to access the libz-compressed body of the
           data for this piece.  
      </ul>
   </ul>
</ul>

 */

/** \page  Algorithms  Algorithms 

<ul>
<li> Creating a new generation
   <ul>
   <li> Scan the file tree, generating a list of file names, sizes, and
        timestamps.
   <li> For each file in this list:
      <ul>
      <li> Lookup the filename, and compare the size and timestamp to the
           stored information.
      <li> If size and timestamp match
         <ul> 
         <li> Update database to indicate the same version of this file
              exists in this new generation as the previous generation.
         </ul>
      <li> else
         <ul>
         <li> 
         </ul>
      </ul>
   </ul>
<li> Deleting a generation
   <ul>
   <li> Step 1
   </ul>
<li> Extraction of a generation
   <ul>
   <li> Step 1
   </ul>
</ul>

 */
