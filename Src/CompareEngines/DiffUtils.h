/**
 * @file  DiffUtils.h
 *
 * @brief Declaration of DiffUtils class.
 */
#pragma once

#include "DiffWrapper.h"

namespace CompareEngines
{

/**
 * @brief A class wrapping diffutils as compare engine.
 *
 * This class needs to have all its data as local copies, not as pointers
 * outside. Lifetime can vary certainly be different from unrelated classes.
 * Filters list being an exception - pcre structs are too complex to easily
 * copy so we'll only keep a pointer to external list.
 */
class DiffUtils : public CDiffWrapper
{
public:
	DiffUtils(const CDiffContext *);
	int diffutils_compare_files(file_data *);

	int m_ndiffs; /**< Real diffs found. */
	int m_ntrivialdiffs; /**< Ignored diffs found. */
};

} // namespace CompareEngines
