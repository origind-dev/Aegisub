// Copyright (c) 2010, Amar Takhar <verm@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

/// @file charset.cpp
/// @brief Character set detection and manipulation utilities.
/// @ingroup libaegisub

#include "libaegisub/charset.h"

#include "libaegisub/file_mapping.h"
#include "libaegisub/scoped_ptr.h"

#ifdef WITH_UCHARDET
#include <uchardet/uchardet.h>
#endif

namespace agi { namespace charset {
std::string Detect(agi::fs::path const& file) {
	agi::read_file_mapping fp(file);

	// If it's over 100 MB it's either binary or big enough that we won't
	// be able to do anything useful with it anyway
	if (fp.size() > 100 * 1024 * 1024)
		return "binary";

	uint64_t binaryish = 0;

#ifdef WITH_UCHARDET
	agi::scoped_holder<uchardet_t> ud(uchardet_new(), uchardet_delete);
	for (uint64_t offset = 0; offset < fp.size(); ) {
		auto read = std::min<uint64_t>(4096, fp.size() - offset);
		auto buf = fp.read(offset, read);
		uchardet_handle_data(ud, buf, read);
		uchardet_data_end(ud);
		if (*uchardet_get_charset(ud))
			return uchardet_get_charset(ud);

		offset += read;

		// A dumb heuristic to detect binary files
		for (size_t i = 0; i < read; ++i) {
			if ((unsigned char)buf[i] < 32 && (buf[i] != '\r' && buf[i] != '\n' && buf[i] != '\t'))
				++binaryish;
		}

		if (binaryish > offset / 8)
			return "binary";
	}
	return uchardet_get_charset(ud);
#else
	auto read = std::min<uint64_t>(4096, fp.size());
	auto buf = fp.read(0, read);
	for (size_t i = 0; i < read; ++i) {
		if ((unsigned char)buf[i] < 32 && (buf[i] != '\r' && buf[i] != '\n' && buf[i] != '\t'))
			++binaryish;
	}

	if (binaryish > read / 8)
		return "binary";
	return "utf-8";
#endif
}
} }
