// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DataItem.h"
#include <Poco/RWLock.h>

namespace Mantid::Kernel {

/** Default constructor
 */
DataItem::DataItem() : m_lock(std::make_unique<Poco::RWLock>()) {}

/** Copy constructor
 * Always makes a unique lock
 */
DataItem::DataItem(const DataItem & /*other*/) : m_lock(std::make_unique<Poco::RWLock>()) {}

/**
 * Destructor. Required in cpp do avoid linker errors when other projects try to
 * inherit from DataItem
 */
DataItem::~DataItem() {}

void DataItem::readLock() { getLock()->readLock(); }
void DataItem::unlock() { getLock()->unlock(); }

/** Private method to access the RWLock object.
 *
 * @return the RWLock object.
 */
Poco::RWLock *DataItem::getLock() const { return m_lock.get(); }

} // namespace Mantid::Kernel
