// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/browser/fileapi/sandbox_isolated_origin_database.h"
#include "webkit/browser/fileapi/sandbox_origin_database.h"

namespace fileapi {

namespace {
const base::FilePath::CharType kOriginDirectory[] = FILE_PATH_LITERAL("iso");
}  // namespace

TEST(SandboxIsolatedOriginDatabaseTest, BasicTest) {
  base::ScopedTempDir dir;
  ASSERT_TRUE(dir.CreateUniqueTempDir());

  std::string kOrigin("origin");
  SandboxIsolatedOriginDatabase database(kOrigin, dir.path(),
                                         base::FilePath(kOriginDirectory));

  EXPECT_TRUE(database.HasOriginPath(kOrigin));

  base::FilePath path1, path2;

  EXPECT_FALSE(database.GetPathForOrigin(std::string(), &path1));
  EXPECT_FALSE(database.GetPathForOrigin("foo", &path1));

  EXPECT_TRUE(database.HasOriginPath(kOrigin));
  EXPECT_TRUE(database.GetPathForOrigin(kOrigin, &path1));
  EXPECT_TRUE(database.GetPathForOrigin(kOrigin, &path2));
  EXPECT_FALSE(path1.empty());
  EXPECT_FALSE(path2.empty());
  EXPECT_EQ(path1, path2);
}

}  // namespace fileapi