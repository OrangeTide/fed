
				    FED 2.24
				    --------


   By Shawn Hargreaves
   http://www.talula.demon.co.uk/


   FED is a folding text editor for MS-DOS, Linux, and Windows. It features:

   - fast and intuitive user interface.
   - color syntax highlighting.
   - can fold blocks of text out of sight, based on code indentation.
   - multiple levels of undo and redo.
   - incremental 'as you type' searching.
   - search/replace through multiple files.
   - browse function to quickly find all references to a symbol.
   - automatic compiler error location.
   - context-sensitive access to external help systems.
   - flexible wordwrap, which correctly handles indented blocks of text.
   - block indent/unindent.
   - binary and hex editing modes for hacking executable and data files.
   - record/playback keystroke macros.
   - built in tetris game and screensaver.
   - configuration options to alter key bindings, screen colors, etc.


   What's new in version 2.24
   --------------------------
   - Robert Riebisch fixed `#...' handling in `makehelp' utility.


   What's new in version 2.23
   --------------------------

   - Robert Riebisch fixed djgpp compile issue with function pch().


   What's new in version 2.22
   --------------------------

   - Proper rendering for box UI characters in the Windows version.
   - Fixed multiprocessor thread race condition in the Windows version.
   - File open checks FED_INCLUDE as well as INCLUDE in the environment.
   - Correct syntax highlighting for 1.0f floating point constants.
   - realtabs option overrides tab mode per file extension.


   What's new in version 2.21
   --------------------------

   - Drag and drop support in the Windows version.


   What's new in version 2.2
   -------------------------

   - Windows port.
   - Tab size can be set differently for each file extension in fed.syn.


   What's new in version 2.16
   --------------------------

   - Added optional wrap column parameter to the -w switch.
   - Now understands ~ for home directories in Unix.


   What's new in version 2.15
   --------------------------

   - Understands Unix-style #! /bin/cmd magic for identifying file types.
   - Implemented clipboard functions on Unix (using a temp file).


   What's new in version 2.14
   --------------------------

   - External tools can now be used as filters for transforming text.
   - Can automatically look in subdirectories when searching for a file.
   - The wordwrap routine now understands what to do with quoted emails.
   - Now works properly in an xterm, including shift keys and mouse input.
   - Numerous bugfixes.


   What's new in version 2.13
   --------------------------

   - Bugfixes.


   What's new in version 2.12
   --------------------------

   - Bugfixes.


   What's new in version 2.11
   --------------------------

   - Ported to use the Allegro library, providing hires 'text' modes.
   - Bugfixes.


   What's new in version 2.1
   -------------------------

   - Ported to Linux, using the Curses package.
   - Added functions to communicate with the Windows clipboard.
   - When run under Windows, the title now reflects the current filename.
   - Released source code under the GPL.


   Copyright
   ---------

   FED is copyright Shawn Hargreaves.

   This program is free, open-source software. It can be redistributed 
   and/or modified under the terms of the GNU General Public License as 
   published by the Free Software Foundation, either version 2 of the 
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but 
   WITHOUT ANY WARRANTY, without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General 
   Public License for more details.

   You should have received a copy of the GNU General Public License along 
   with this program in the file COPYING, if not, write to the Free Software 
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

