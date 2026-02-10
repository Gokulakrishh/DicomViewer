***************************************************************
                syngo (R) fastView Read Me
***************************************************************

This file contains general information about syngo fastView 
and last-minute information about version VX57N. 
We recommend reading this entire file before using syngo fastView.

Unless otherwise noted, all materials provided in this version of 
syngo fastView are copyright (C) 2016 by 
Siemens Healthcare GmbH, Erlangen, Germany.

TABLE OF CONTENTS
===============================================================
  1. GENERAL NOTES
  2. RESTRICTIONS
  3. WHERE TO FIND INFORMATION AND CONTACT
  4. THE LEGAL DISCLAIMER


1. GENERAL NOTES
===============================================================
What is syngo fastView:

syngo fastView is not a medical device and therefore is not permitted for diagnostic use.
For additional information please refer to the online help.

syngo fastView is a standalone viewing tool for DICOM images: 
- Visualization of DICOM images (series, studies, patients)
- General display functions like Stripe and Stack Mode, Movie, Zoom, Pan, Window
- 3D Multiplanar Reconstruction of CT and MR data sets
- Conversion functions like Save as Bitmap, Save as JPEG, Save as AVI.
- Special display functions like Ultrasound Stress Echo Mode, Basic 3 D Mode, 
  Basic Fusion Mode, DSA native and subtracted Mode
- syngo compatible. 

DICOM is a standard: 
Digital Imaging & Communications in Medicine. 
Further information about DICOM:  http://www.nema.org
DICOM is used to supply interchangeability of medical information and images. 

New in release VX57M :
- Pixel Lens function for the inspection of image grey values is provided
- Sorting of Series in Reverse Order is possible via the menu entry ‘Reverse Series Order’ under the Sort menu.
- Sorting of MR data according to slice position or instance number is possible
- Dicom SOP class “Enhanced CT Image Storage” and “Digital X-Ray Image Storage – for Presentation” is supported

syngo fastView is tested on following processors: 
Intel Pentium, Intel Centrino, Intel Pentium D and Intel Pentium Dual Core.
syngo fastView is not tested on other processors like Intel Xeon, Intel Itanium etc.
syngo fastView requires a minimum RAM configuration of 1GB.
syngo fastView supports file formats FAT, FAT32, NTFS.

Copies of clinical images together with syngo fastView can be handed over 
to patients on CD/DVD/BD for private use only.


2. RESTRICTIONS
===============================================================
General restrictions
-------------------------

- Printer options need to be set to “Fit to Printer Margins” or 
  “Fit to Size” to ensure WYSIWYG in the printout.

- “Microsoft document image writer” as printer is not supported.

- AVI generation can fail in certain workflow and dataset combinations after repeated attempts. 
  The workaround for this issue is to try the same in a different layout.

- If Codec files are missing, AVI files created in 16:1 layout in Windows 7  Ultimate Edition (Win 7 UE) cannot be played with Windows Media Player (WMP). 
  Workaround for this issue is to install codec files.

- DSA subtraction functionality is not supported for lossy compressed images.

- syngo FastView is scanning directories on CD/DVD and accepts also files with extension 
  “.bmp” or “.jpg” as valid input files. 
  In case other files than medical images with these extensions (i.e. logos) have been stored, they are also displayed. 
  In this case select the patient manually from the left area of the fastView navigator to display the images.

- Recording of offline files is only working, 
  if a DicomDir file is also available in the same offline directory.

- DICOM SOP classes, which are not supported, normally cannot be loaded into the Viewer. For some unsupported SOP classes such as 
  e.g. DICOM SR, syngo fastView nevertheless tries to load these objects unsuccessfully and a blank image might be displayed

- To run fastView from a Chinese/Japanese folder, it is always recommended to set the regional and advanced language settings to Chinese\Japanese. 
  If the Local language settings is 'Chinese / Japanese' and advanced (system locale) language is set to 'English', 
  it is recommended to install fastView in a folder which does not contain any Unicode characters 
  and open the images from a folder not containing Unicode characters.

- Recommendation to display Chinese/Japanese characters properly in "Dicom Header Display":
  If the Local language settings is 'Chinese / Japanese' and advanced (system locale) language is 'English',
  the "Dicom Header Display" does not show Chinese / Japanese characters properly. 
  This is a known limitation and it is recommended to set the regional and advanced language settings to Chinese\Japanese to handle such cases.



2D viewing restrictions
-----------------------------

- DICOM overlay graphics displayed at certain zoom levels might appear to be cut at some places for images with thin graphics. 
  This is due to the fact that syngo fastView does interpolation during resampling of the pixel data. 
  The workaround for this issue is to zoom in, then DICOM overlay graphics are displayed properly.

- Use of function Magic Class while Movie mode is running is not supported and will stop Movie mode.

3D viewing restrictions
-----------------------------

- 3D Multiplanar Reconstruction (MPR) of images acquired with Gantry/Detector Tilt or Patient Orientation Tilt is not supported.

- 3D Multiplanar Reconstruction (MPR) might be limited for image data when DICOM Tag ‘Image Orientation Patient’ (0020,0037) has another value than 1 or 0.

- Display of Orientation Cube in 3D MPR is not always correct.

- NM multiframes are not supported for 3D Mode.

- The syngo fastView window needs to be maximized in 3DMPR and Fusion modes to ensure WYSIWYG in the Printout and for the SaveAs function.



2. WHERE TO FIND INFORMATION AND CONTACT
=======================================================
Information Files
---------------------

Refer to the file Help\INDEX.HTM in the install directory 
for a detailed description of syngo fastView's capabilities.

For further information please visit the syngo fastView Page
www.healthcare.siemens.com/medical-imaging-it/syngo-special-topics/syngo-fastview

Online Help is available, too. Please press F1 key or choose 
Help | Help from the main menu.


Online Resources
--------------------
Internet:
	http://www.syngo.com


3. LEGAL DISCLAIMER
=======================================================
THE INFORMATION AND CODE PROVIDED HEREUNDER (COLLECTIVELY 
REFERRED TO AS "SOFTWARE") IS PROVIDED AS IS WITHOUT 
WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL 
THE AUTHOR BE LIABLE FOR ANY DAMAGES WHATSOEVER INCLUDING 
DIRECT, INDIRECT, INCIDENTAL, CONSEQUENTIAL, LOSS OF 
BUSINESS PROFITS OR SPECIAL DAMAGES, EVEN IF THE AUTHOR 
HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. 

Refer to readme.rtf in your local fastView folder for END-USER LICENSE AGREEMENT.

syngo(R) is a registered trademark owned by Siemens Healthcare GmbH.
