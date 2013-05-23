/***************************************************************************************************
 *  Copyright 2013 maidsafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use. The use of this code is governed by the licence file licence.txt found in the root of     *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit written *
 *  permission of the board of directors of MaidSafe.net.                                          *
 **************************************************************************************************/

#include "maidsafe/data_types/structured_data_version.h"

#include <limits>

#include "maidsafe/common/error.h"
#include "maidsafe/common/on_scope_exit.h"

#include "maidsafe/data_types/structured_data_version.pb.h"


namespace maidsafe {

StructuredDataVersions::VersionName::VersionName()
    : index(std::numeric_limits<uint64_t>::max()),
      id() {}

StructuredDataVersions::VersionName::VersionName(uint32_t index_in,
                                                 const ImmutableData::name_type& id_in)
    : index(index_in),
      id(id_in) {}

StructuredDataVersions::VersionName::VersionName(const VersionName& other)
    : index(other.index),
      id(other.id) {}

StructuredDataVersions::VersionName::VersionName(VersionName&& other)
    : index(std::move(other.index)),
      id(std::move(other.id)) {}

StructuredDataVersions::VersionName& StructuredDataVersions::VersionName::operator=(
    VersionName other) {
  swap(*this, other);
  return *this;
}

void swap(StructuredDataVersions::VersionName& lhs,
          StructuredDataVersions::VersionName& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.index, rhs.index);
  swap(lhs.id, rhs.id);
}

bool operator==(const StructuredDataVersions::VersionName& lhs,
                const StructuredDataVersions::VersionName& rhs) {
  return std::tie(lhs.index, lhs.id) == std::tie(rhs.index, rhs.id);
}

bool operator!=(const StructuredDataVersions::VersionName& lhs,
                const StructuredDataVersions::VersionName& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const StructuredDataVersions::VersionName& lhs,
               const StructuredDataVersions::VersionName& rhs) {
  return std::tie(lhs.index, lhs.id) < std::tie(rhs.index, rhs.id);
}

bool operator>(const StructuredDataVersions::VersionName& lhs,
               const StructuredDataVersions::VersionName& rhs) {
  return operator< (rhs, lhs);
}

bool operator<=(const StructuredDataVersions::VersionName& lhs,
                const StructuredDataVersions::VersionName& rhs) {
  return !operator> (lhs, rhs);
}

bool operator>=(const StructuredDataVersions::VersionName& lhs,
                const StructuredDataVersions::VersionName& rhs) {
  return !operator< (lhs, rhs);
}



StructuredDataVersions::Details::Details() : parent(), children() {}

StructuredDataVersions::Details::Details(const Details& other)
    : parent(other.parent),
      children(other.children) {}

StructuredDataVersions::Details::Details(Details&& other)
    : parent(std::move(other.parent)),
      children(std::move(other.children)) {}

StructuredDataVersions::Details& StructuredDataVersions::Details::operator=(Details other) {
  swap(*this, other);
  return *this;
}

void swap(StructuredDataVersions::Details& lhs,
          StructuredDataVersions::Details& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.parent, rhs.parent);
  swap(lhs.children, rhs.children);
}



StructuredDataVersions::StructuredDataVersions(uint32_t max_versions, uint32_t max_branches)
    : max_versions_(max_versions),
      max_branches_(max_branches),
      versions_(),
      root_(std::make_pair(VersionName(), std::end(versions_))),
      tips_of_trees_(),
      orphans_() {
  if (max_versions_ < 1U || max_branches_ < 1U)
    ThrowError(CommonErrors::invalid_parameter);
}

StructuredDataVersions::StructuredDataVersions(const StructuredDataVersions& other)
    : max_versions_(other.max_versions_),
      max_branches_(other.max_branches_),
      versions_(other.versions_),
      root_(other.root_),
      tips_of_trees_(other.tips_of_trees_),
      orphans_(other.orphans_) {}

StructuredDataVersions::StructuredDataVersions(StructuredDataVersions&& other)
    : max_versions_(std::move(other.max_versions_)),
      max_branches_(std::move(other.max_branches_)),
      versions_(std::move(other.versions_)),
      root_(std::move(other.root_)),
      tips_of_trees_(std::move(other.tips_of_trees_)),
      orphans_(std::move(other.orphans_)) {}


StructuredDataVersions& StructuredDataVersions::operator=(StructuredDataVersions other) {
  swap(*this, other);
  return *this;
}

StructuredDataVersions::StructuredDataVersions(const serialised_type& serialised_data_versions)
    : max_versions_(),
      max_branches_(),
      versions_(),
      root_(),
      tips_of_trees_(),
      orphans_() {
  protobuf::StructuredDataVersions proto_versions;
  if (!proto_versions.ParseFromString(serialised_data_versions->string()))
    ThrowError(CommonErrors::parsing_error);

}

StructuredDataVersions::serialised_type StructuredDataVersions::Serialise() const {
  protobuf::StructuredDataVersions proto_versions;


  return serialised_type(NonEmptyString(proto_versions.SerializeAsString()));
}

void StructuredDataVersions::ApplySerialised(const serialised_type& serialised_data_versions) {
  StructuredDataVersions new_info(serialised_data_versions);

}

void StructuredDataVersions::Put(const VersionName& old_version, const VersionName& new_version) {
  if (NewVersionPreExists(old_version, new_version))
    return;

  // Check we've not been asked to store two roots.
  bool is_root(!old_version.id->IsInitialised());
  if (is_root && root_.second != std::end(versions_) && !RootParentName().id->IsInitialised())
    ThrowError(CommonErrors::invalid_parameter);

  // Construct temp objects before modifying members in case exception is thrown.
  Version version(std::make_pair(new_version, std::make_shared<Details>()));
  version.second->parent = (is_root ? std::end(versions_) : versions_.find(old_version));

  bool is_orphan(version.second->parent == std::end(versions_) && !is_root);
  auto orphans_range(orphans_.equal_range(new_version));
    //check we can't iterate back to ourself (avoid circular parent-child chain)

  for (auto orphan_itr(orphans_range.first); orphan_itr != orphans_range.second; ++orphan_itr)
    version.second->children.push_back(orphan_itr->second);

  auto unorphaned_count(std::distance(orphans_range.first, orphans_range.second));
  bool unorphans_existing_root(root_.first.id->IsInitialised() && RootParentName() == old_version);
  assert(!(unorphaned_count != 0 && unorphans_existing_root));

  // Handle case where we're about to exceed 'max_versions_'.
  bool erase_existing_root(false);
  if (AtVersionsLimit()) {
    if (unorphans_existing_root || is_root)
      // This new version would become 'root_', only to be immediately erased to bring version count
      // back down to 'max_versions_'.
      return;
    erase_existing_root = true;
  }

  // Handle case where we're about to exceed 'max_branches_'.
  if (AtBranchesLimit() && unorphaned_count == 0) {
    if (is_orphan || !version.second->parent->second->children.empty()) {
      // We're going to exceed limit - see if deleting 'root_' helps
      bool root_is_tip_of_tree(root_.second != std::end(versions_) &&
                               root_.second->second->children.empty());
      if (root_is_tip_of_tree)
        erase_existing_root = true;
      else
        ThrowError(CommonErrors::cannot_exceed_limit);
    }
  }

  auto inserted_itr(versions_.insert(version).first);
  if (!is_root && !is_orphan)
    version.second->parent->second->children.push_back(inserted_itr);

  //do unorphaning
  //for each unorphaning:
  //  check we can't iterate back to ourself (avoid circular parent-child chain)

  //mark as root
}

StructuredDataVersions::VersionName StructuredDataVersions::ParentName(VersionsItr itr) const {
  return itr->second->parent->first;
}

StructuredDataVersions::VersionName StructuredDataVersions::ParentName(
    Orphans::iterator itr) const {
  return itr->first;
}

StructuredDataVersions::VersionName StructuredDataVersions::RootParentName() const {
  return root_.first;
}

bool StructuredDataVersions::NewVersionPreExists(const VersionName& old_version,
                                                 const VersionName& new_version) const {
  auto existing_itr(versions_.find(new_version));
  if (existing_itr != std::end(versions_)) {
    if (existing_itr->second->parent->first == old_version)
      return true;
    else
      ThrowError(CommonErrors::invalid_parameter);
  }
  return false;
}

std::pair<StructuredDataVersions::Orphans::const_iterator,
          StructuredDataVersions::Orphans::const_iterator>
    StructuredDataVersions::GetUnorphanGroup(const Version& version) const {
  auto orphans_range(orphans_.equal_range(version.first));
  for (auto orphan_itr(orphans_range.first); orphan_itr != orphans_range.second; ++orphan_itr) {
    // Check we can't iterate back to ourself (avoid circular parent-child chain)
    CheckVersionNotInBranch(orphan_itr->second, version.first);
  }
  return orphans_range;
}

void StructuredDataVersions::CheckVersionNotInBranch(VersionsItr itr,
                                                     const VersionName& version) const {
  // TODO(Fraser#5#): 2013-05-23 - This is probably a good candidate for parallelisation (futures)
  for (auto child_itr : itr->second->children) {
    if (child_itr->first == version)
      ThrowError(CommonErrors::invalid_parameter);
    CheckVersionNotInBranch(child_itr, version);
  }
}





std::vector<StructuredDataVersions::VersionName> StructuredDataVersions::Get() const {
  std::vector<StructuredDataVersions::VersionName> result;
  for (const auto& tot : tips_of_trees_) {
    assert(tot->second->child_count == 0U);
    result.push_back(tot->first);
  }
  return result;
}

std::vector<StructuredDataVersions::VersionName> StructuredDataVersions::GetBranch(
    const VersionName& /*branch_tip*/) const {
  //auto branch_tip_itr(FindBranchTip(branch_tip));
  //CheckBranchTipIterator(branch_tip, branch_tip_itr);
  //auto itr(*branch_tip_itr);
  std::vector<StructuredDataVersions::VersionName> result;
  //while (itr != std::end(versions_)) {
  //  result.push_back(itr->first);
  //  itr = itr->second->parent;
  //}
  return result;
}

void StructuredDataVersions::DeleteBranchUntilFork(const VersionName& /*branch_tip*/) {
  //auto branch_tip_itr(FindBranchTip(branch_tip));
  //CheckBranchTipIterator(branch_tip, branch_tip_itr);
  //auto itr(*branch_tip_itr);
  //tips_of_trees_.erase(branch_tip_itr);

  //for (;;) {
  //  auto parent_itr = itr->second->parent;
  //  if (parent_itr == std::end(versions_)) {
  //    // Found root or orphan.  Either way, we're at the end of the branch.
  //    EraseRootOrOrphanOfBranch(itr);
  //    versions_.erase(itr);
  //    return;
  //  }

  //  versions_.erase(itr);
  //  if (--parent_itr->second->child_count > 0U)
  //    return;  // Found fork.

  //  itr = parent_itr;
  //}
}

void StructuredDataVersions::clear() {
  versions_.clear();
  root_ = std::make_pair(VersionName(), std::end(versions_));
  tips_of_trees_.clear();
  orphans_.clear();
}
/*
void StructuredDataVersions::EraseRootOrOrphanOfBranch(VersionsItr itr) {
  assert(itr->second.parent == std::end(versions_));
  if (itr == root_.second) {
    // If we're erasing root, try to assign an orphan as the new root.
    if (orphans_.empty()) {
      root_ = std::make_pair(VersionName(), std::end(versions_));
    } else {
      auto replacement_itr(FindReplacementRootFromCurrentOrphans());
      root_ = *replacement_itr;
      orphans_.erase(replacement_itr);
    }
  } else {
    orphans_.erase(std::remove_if(std::begin(orphans_),
                                  std::end(orphans_),
                                  [itr](const Orphans::value_type& orphan) {
                                      return orphan.second == itr;
                                  }),
                   std::end(orphans_));
  }
}

StructuredDataVersions::Orphans::iterator
    StructuredDataVersions::FindReplacementRootFromCurrentOrphans() {
  auto lower_itr(std::begin(orphans_));
  auto upper_itr(orphans_.upper_bound(lower_itr->first));
  for (;;) {
    // If this key in the multimap has only one value, return this.
    if (std::distance(lower_itr, upper_itr) == 1)
      return lower_itr;
    // If we failed to find a unique key, just return the first element.
    if (upper_itr == std::end(orphans_))
      return std::begin(orphans_);
    // Advance to the next key.
    lower_itr = upper_itr;
    upper_itr = orphans_.upper_bound(lower_itr->first);
  }
}

void StructuredDataVersions::InsertRoot(const VersionName& root_name) {
  // Construct temp object before modifying members in case make_pair throws.
  Version root_version(std::make_pair(root_name, Details()));
  root_version.second.parent = std::end(versions_);

  if (versions_.size() == 1U && max_versions_ == 1U)
    versions_.clear();

  if (versions_.empty()) {
    versions_.insert(root_version);
    root_ = std::begin(versions_);
    tips_of_trees_.push_back(std::begin(versions_));
    return;
  }

  // This is only valid if root_ == end().
  if (root_ != std::end(versions_))
    ThrowError(CommonErrors::invalid_parameter);

  auto orphan_itr(FindOrphanOf(root_name));
  bool will_create_new_branch(orphan_itr == std::end(orphans_));
  if (will_create_new_branch && AtBranchesLimit())
    ThrowError(CommonErrors::cannot_exceed_limit);

  if (AtVersionsLimit()) {
    // If the new root was inserted, we'd exceed max_versions_, and then we'd remove the root as
    // normal to bring the count within the limit again.  Skip the insert and if the new root is the
    // parent of an orphan, mark the orphan as the root.
    if (orphan_itr == std::end(orphans_))
      return;
    orphan_itr->second->second.parent = std::end(versions_);
    root_ = orphan_itr->second;
    orphans_.erase(orphan_itr);
  } else {
    // This should always succeed since we've already checked the new version doesn't exist.
    auto result(versions_.insert(root_version));
    assert(result.second);
    root_ = result.first;
    if (orphan_itr != std::end(orphans_)) {
      orphan_itr->second->second.parent = root_;
      orphans_.erase(orphan_itr);
    }
  }
}

void StructuredDataVersions::InsertChild(const VersionName& child_name, VersionsItr parent_itr) {
  // Construct temp object before modifying members in case make_pair throws.
  Version child_version(std::make_pair(child_name, Details()));
  child_version.second.parent = parent_itr;

  //check we aren't going to exceed the limits

  //see if we've un-orphaned any orphans

}

void StructuredDataVersions::InsertOrphan(const VersionName& child_name,
                                          const VersionName& parent_name) {
  // See if we're going to "un-orphan" any orphans or 'root_'.
  auto orphans_range(orphans_.equal_range(child_name));
  auto unorphaned_count(std::distance(orphans_range.first, orphans_range.second));
  bool unorphans_root(unorphaned_count == 0 ?
                      root_.first->IsInitialised() && root_.first == child_name : false);

  bool is_new_root(!parent_name->IsInitialised() || unorphans_root);
  if (is_new_root)
    CheckCanInsertRoot();
  else
    CheckCanInsertOrphan();

  // Construct temp objects before modifying members in case make_pair throws.
  Version child_version(std::make_pair(child_name, Details()));
  child_version.second.parent = std::end(versions_);

  auto inserted_itr(versions_.insert(child_version).first);

  // See if we've un-orphaned any orphans
  Unorphan(inserted_itr, orphans_range);

  // Remove root if necessary

}

void StructuredDataVersions::CanRemoveRootIfRequired(bool new_root_creates_branch) const {
  bool existing_root_can_be_replaced(root_.second == std::end(versions_) ?
                                     true :
                                     root_.second->second.child_count < 2U);
  if (AtBranchesLimit() && !existing_root_can_be_replaced)
    ThrowError(CommonErrors::cannot_exceed_limit);
  if (AtVersionsLimit() && !existing_root_can_be_replaced)
    ThrowError(CommonErrors::unable_to_handle_request);
}

void StructuredDataVersions::CheckCanInsertOrphan() const {
  if (AtBranchesLimit())
    ThrowError(CommonErrors::cannot_exceed_limit);
  if (AtVersionsLimit())
    ThrowError(CommonErrors::unable_to_handle_request);
}

void StructuredDataVersions::Unorphan(
    VersionsItr parent,
    std::pair<Orphans::iterator, Orphans::iterator> orphans_range) {
  while (orphans_range.first != orphans_range.second) {
    orphans_range.first->second->second->parent = parent;
    ++parent->second->child_count;
    orphans_range.first = orphans_.erase(orphans_range.first);
  }

  if we've just unorphaned > 1 and this possible parent is going to be new root, and we're also
  going to exceed version count, fail.

  // Check 'root_'.  'root_' probably doesn't have an intialised parent name, but if it was set via
  // replacing an existing one from the list of orphans, it might have a missing parent.
  if (root_.first->IsInitialised() && root_.first == possible_parent->first) {
    if we're going to insert
  }

    check we can't iterate back to ourself (avoid circular parent-child chain)
}

std::vector<StructuredDataVersions::VersionsItr>::iterator StructuredDataVersions::FindBranchTip(
    const VersionName& name) {
  return std::find_if(std::begin(tips_of_trees_), std::end(tips_of_trees_),
                      [&name](VersionsItr branch_tip) { return branch_tip->first == name; });
}

std::vector<StructuredDataVersions::VersionsItr>::const_iterator
    StructuredDataVersions::FindBranchTip(const VersionName& name) const {
  return std::find_if(std::begin(tips_of_trees_), std::end(tips_of_trees_),
                      [&name](VersionsItr branch_tip) { return branch_tip->first == name; });
}

void StructuredDataVersions::CheckBranchTipIterator(
    const VersionName& name,
    std::vector<VersionsItr>::const_iterator branch_tip_itr) const {
  if (branch_tip_itr == std::end(tips_of_trees_)) {
    if (versions_.find(name) == std::end(versions_))
      ThrowError(CommonErrors::no_such_element);
    else
      ThrowError(CommonErrors::invalid_parameter);
  }
}

std::vector<StructuredDataVersions::Orphan>::iterator StructuredDataVersions::FindOrphanOf(
    const VersionName& name) {
  return std::find_if(std::begin(orphans_), std::end(orphans_),
                      [&name](const Orphan& orphan) { return orphan.first == name; });
}

std::vector<StructuredDataVersions::Orphan>::const_iterator StructuredDataVersions::FindOrphanOf(
    const VersionName& name) const {
  return std::find_if(std::begin(orphans_), std::end(orphans_),
                      [&name](const Orphan& orphan) { return orphan.first == name; });
}
*/
bool StructuredDataVersions::AtVersionsLimit() const {
  assert(versions_.size() <= max_versions_);
  return versions_.size() == max_versions_;
}

bool StructuredDataVersions::AtBranchesLimit() const {
  assert(tips_of_trees_.size() <= max_branches_);
  return tips_of_trees_.size() == max_branches_;
}

void swap(StructuredDataVersions& lhs, StructuredDataVersions& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.max_versions_, rhs.max_versions_);
  swap(lhs.max_branches_, rhs.max_branches_);
  swap(lhs.versions_, rhs.versions_);
  swap(lhs.root_, rhs.root_);
  swap(lhs.tips_of_trees_, rhs.tips_of_trees_);
  swap(lhs.orphans_, rhs.orphans_);
}

}  // namespace maidsafe
