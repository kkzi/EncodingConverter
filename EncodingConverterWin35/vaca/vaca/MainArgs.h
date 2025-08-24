// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2010 David Capello
//
// This file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#ifndef VACA_MAINARGS_H
#define VACA_MAINARGS_H

#include "vaca/base.h"

namespace vaca {

  namespace details {
    class VACA_DLL MainArgs
    {
    public:
      static void setArgs(int, char**);
    };
  }

} // namespace vaca

#endif // VACA_MAINARGS_H
