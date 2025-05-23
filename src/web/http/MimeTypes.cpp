/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "web/http/MimeTypes.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <filesystem>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

#ifdef HAS_LIBMAGIC
    magic_t MimeTypes::magic;
#endif

    std::map<std::string, std::string> MimeTypes::mimeType = {
        {".dwg", "application/acad"},
        {".asd", "application/astound"},
        {".asn", "application/astound"},
        {".tsp", "application/dsptype"},
        {".dxf", "application/dxf"},
        {".reg", "application/force-download"},
        {".spl", "application/futuresplash"},
        {".gz", "application/gzip"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".ptlk", "application/listenup"},
        {".hqx", "application/mac-binhex40"},
        {".mbd", "application/mbedlet"},
        {".mif", "application/mif"},
        {".xls", "application/msexcel"},
        {".xla", "application/msexcel"},
        {".hlp", "application/mshelp"},
        {".chm", "application/mshelp"},
        {".ppt", "application/mspowerpoint"},
        {".ppz", "application/mspowerpoint"},
        {".pps", "application/mspowerpoint"},
        {".pot", "application/mspowerpoint"},
        {".doc", "application/msword"},
        {".dot", "application/msword"},
        {".bin", "application/octet-stream"},
        {".file", "application/octet-stream"},
        {".com", "application/octet-stream"},
        {".class", "application/octet-stream"},
        {".ini", "application/octet-stream"},
        {".oda", "application/oda"},
        {".pdf", "application/pdf"},
        {".ai", "application/postscript"},
        {".eps", "application/postscript"},
        {".ps", "application/postscript"},
        {".rtc", "application/rtc"},
        {".rtf", "application/rtf"},
        {".smp", "application/studiom"},
        {".tbk", "application/toolbook"},
        {".vmd", "application/vocaltec-media-desc"},
        {".vmf", "application/vocaltec-media-file"},
        {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {".xml", "application/xml"},
        {".bcpio", "application/x-bcpio"},
        {".z", "application/x-compress"},
        {".cpio", "application/x-cpio"},
        {".csh", "application/x-csh"},
        {".dcr", "application/x-director"},
        {".dir", "application/x-director"},
        {".dxr", "application/x-director"},
        {".dvi", "application/x-dvi"},
        {".evy", "application/x-envoy"},
        {".gtar", "application/x-gtar"},
        {".hdf", "application/x-hdf"},
        {".php", "application/x-httpd-php"},
        {".phtml", "application/x-httpd-php"},
        {".latex", "application/x-latex"},
        {".bin", "application/x-macbinary"},
        {".mif", "application/x-mif"},
        {".nc", "application/x-netcdf"},
        {".cdf", "application/x-netcdf"},
        {".nsc", "application/x-nschat"},
        {".sh", "application/x-sh"},
        {".shar", "application/x-shar"},
        {".swf", "application/x-shockwave-flash"},
        {".cab", "application/x-shockwave-flash"},
        {".spr", "application/x-sprite"},
        {".sprite", "application/x-sprite"},
        {".sit", "application/x-stuffit"},
        {".sca", "application/x-supercard"},
        {".sv4cpio", "application/x-sv4cpio"},
        {".sv4crc", "application/x-sv4crc"},
        {".tar", "application/x-tar"},
        {".tcl", "application/x-tcl"},
        {".tex", "application/x-tex"},
        {".texinfo", "application/x-texinfo"},
        {".texi", "application/x-texinfo"},
        {".t", "application/x-troff"},
        {".tr", "application/x-troff"},
        {".roff", "application/x-troff"},
        {".man", "application/x-troff-man"},
        {".troff", "application/x-troff-man"},
        {".me", "application/x-troff-me"},
        {".troff", "application/x-troff-me"},
        {".me", "application/x-troff-ms"},
        {".troff", "application/x-troff-ms"},
        {".ustar", "application/x-ustar"},
        {".src", "application/x-wais-source"},
        {".zip", "application/zip"},
        {".au", "audio/basic"},
        {".snd", "audio/basic"},
        {".es", "audio/echospeech"},
        {".mp3", "audio/mpeg"},
        {".mp4", "audio/mp4"},
        {".ogg", "audio/ogg"},
        {".tsi", "audio/tsplayer"},
        {".vox", "audio/voxware"},
        {".wav", "audio/wav"},
        {".aif", "audio/x-aiff"},
        {".aiff", "audio/x-aiff"},
        {".aifc", "audio/x-aiff"},
        {".dus", "audio/x-dspeeh"},
        {".cht", "audio/x-dspeeh"},
        {".mid", "audio/x-midi"},
        {".midi", "audio/x-midi"},
        {".mp2", "audio/x-mpeg"},
        {".ram", "audio/x-pn-realaudio"},
        {".ra", "audio/x-pn-realaudio"},
        {".rpm", "audio/x-pn-realaudio-plugin"},
        {".stream", "audio/x-qt-stream"},
        {".dwf", "drawing/x-dwf"},
        {".bmp", "image/bmp"},
        {".bmp", "image/x-bmp"},
        {".bmp", "image/x-ms-bmp"},
        {".cod", "image/cis-cod"},
        {".ras", "image/cmu-raster"},
        {".fif", "image/fif"},
        {".gif", "image/gif"},
        {".ief", "image/ief"},
        {".jpeg", "image/jpeg"},
        {".jpg", "image/jpeg"},
        {".jpe", "image/jpeg"},
        {".png", "image/png"},
        {".svg", "image/svg+xml"},
        {".tiff", "image/tiff"},
        {".tif", "image/tiff"},
        {".mcf", "image/vasa"},
        {".wbmp", "image/vnd.wap.wbmp"},
        {".fh4", "image/x-freehand"},
        {".fh5", "image/x-freehand"},
        {".fhc", "image/x-freehand"},
        {".ico", "image/x-icon"},
        {".pnm", "image/x-portable-anymap"},
        {".pbm", "image/x-portable-bitmap"},
        {".pgm", "image/x-portable-graymap"},
        {".ppm", "image/x-portable-pixmap"},
        {".rgb", "image/x-rgb"},
        {".xwd", "image/x-windowdump"},
        {".xbm", "image/x-xbitmap"},
        {".xpm", "image/x-xpixmap"},
        {".wrl", "model/vrml"},
        {".ics", "text/calendar"},
        {".csv", "text/comma-separated-values"},
        {".css", "text/css"},
        {".htm", "text/html"},
        {".html", "text/html"},
        {".shtml", "text/html"},
        {".xhtml", "text/html"},
        {".js", "text/javascript"},
        {".txt", "text/plain"},
        {".rtx", "text/richtext"},
        {".rtf", "text/rtf"},
        {".tsv", "text/tab-separated-values"},
        {".wml", "text/vnd.wap.wml"},
        {".wmlc", "application/vnd.wap.wmlc"},
        {".wmls", "text/vnd.wap.wmlscript"},
        {".wmlsc", "application/vnd.wap.wmlscriptc"},
        {".xml", "text/xml"},
        {".etx", "text/x-setext"},
        {".sgm", "text/x-sgml"},
        {".sgml", "text/x-sgml"},
        {".talk", "text/x-speech"},
        {".spc", "text/x-speech"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".mpe", "video/mpeg"},
        {".mp4", "video/mp4"},
        {".ogg", "video/ogg"},
        {".ogv", "video/ogg"},
        {".qt", "video/quicktime"},
        {".mov", "video/quicktime"},
        {".viv", "video/vnd.vivo"},
        {".vivo", "video/vnd.vivo"},
        {".webm", "video/webm"},
        {".avi", "video/x-msvideo"},
        {".movie", "video/x-sgi-movie"},
        {".vts", "workbook/formulaone"},
        {".vtts", "workbook/formulaone"},
        {".3dmf", "x-world/x-3dmf"},
        {".3dm", "x-world/x-3dmf"},
        {".qd3d", "x-world/x-3dmf"},
        {".qd3", "x-world/x-3dmf"},
        {".wrl", "x-world/x-vrml"}};

    MimeTypes::MimeTypes() {
#ifdef HAS_LIBMAGIC
        MimeTypes::magic = magic_open(MAGIC_MIME);

        if (magic_load(magic, nullptr) != 0) {
            LOG(DEBUG) << "HTTP: Cannot load magic database - " + std::string(magic_error(magic));
            magic_close(magic);
            magic = nullptr;
        }
#endif
    }

    MimeTypes::~MimeTypes() {
#ifdef HAS_LIBMAGIC
        magic_close(MimeTypes::magic);
#endif
    }

    std::string MimeTypes::contentType(const std::string& file) {
        const std::map<std::string, std::string>::iterator it = mimeType.find(std::filesystem::path(file).extension());

        std::string type;

        if (it != mimeType.end()) {
            type = it->second;
        }
#ifdef HAS_LIBMAGIC
        else if (magic != nullptr) {
            const char* magicType = magic_file(MimeTypes::magic, file.c_str());
            type = magicType != nullptr ? magicType : std::string();
        }
#endif

        return !type.empty() ? type : "application/octet-stream";
    }

} // namespace web::http
