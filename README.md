# GDCM fork for Insight Toolkit (ITK)

This project is a fork of the GDCM source hosted at
https://github.com/malaterre/GDCM.git.

It holds the GDCM that ITK vendors. ITK consumes a filtered subtree of this
fork (`Source`, the `Utilities` codecs, and the top-level CMake files) through
`Modules/ThirdParty/GDCM/UpdateFromUpstream.sh`. The overlay carries the fixes
ITK needs that are still under review upstream, plus the build changes that let
GDCM compile inside ITK.

# What is the branch naming convention?

Each fork branch is named with the pattern `for/itk-gdcm-BASE-SHA{7}`

where:
- `BASE` is the upstream GDCM release the overlay is built on (e.g. `3.2.11`),
  or `master` when it is built on the upstream `master` tip.
- `SHA{7}` is the 7-character hash of the overlay tip commit.

For example, `for/itk-gdcm-master-4f20194` carries the overlay on upstream
`master` at `2cd05d132`, with tip commit `4f20194`.

The default branch of this fork is `welcome`, an orphan branch holding only
this document. ITK pins a `for/itk-gdcm-*` branch through the `tag` variable in
`Modules/ThirdParty/GDCM/UpdateFromUpstream.sh`. Each pinned branch is
protected against deletion and force-push; no tag shares its name.

See ITK's
[Third-Party Fork Conventions](https://github.com/InsightSoftwareConsortium/ITK/blob/main/Documentation/Maintenance/ThirdPartyForkConventions.md).

# What the overlay contains

The overlay has two layers, in this order:

1. **Open upstream pull requests.** Each ITK-relevant pull request still open
   on malaterre/GDCM is merged with a `--no-ff` merge commit named
   `Merge malaterre/GDCM#<N>: <title>`, so the topology records which pull
   request each change came from. When a pull request lands upstream, drop it
   on the next update. The goal is an empty first layer.
2. **ITK build overlay.** Commits ITK needs that have no upstream pull request
   yet: routing the codecs through ITK's mangled `itk_*.h` headers, the
   settings for building inside ITK (RLE sources compiled into `gdcmMSFF`,
   socket++ namespaced as `gdcmsocketxx`, `CMAKE_DL_LIBS` and default resource
   paths handled for an embedded build), and an absolute URL for the
   `Testing/Data` submodule (the relative `../gdcmdata` URL has no counterpart
   in this organization).

   Every commit in this layer except the submodule URL is meant to become an
   upstream pull request, generalized where needed (for example, a CMake
   option or a configurable wrapper header instead of a hard-coded ITK name).
   Once its pull request is open, the commit moves to the first layer as a
   merge; once it lands, it leaves the overlay.

# Workflow for Updating

## Setup repositories

1. Clone (or reuse) the fork and ensure the tree is clean:

```
git clone https://github.com/InsightSoftwareConsortium/GDCM.git
cd GDCM
```

2. Wire the upstream remote:

```
git remote add upstream https://github.com/malaterre/GDCM.git
```

3. Fetch:

```
git fetch origin
git fetch upstream
```

## Create a new branch following the convention

4. Choose the upstream base, normally the `master` tip, since the open pull
   requests target it:

```
git fetch upstream master
BASE=upstream/master
git switch -c itk-overlay-wip "${BASE}"
```

5. Merge each open ITK-relevant pull request, one `--no-ff` merge each:

```
for N in <open PR numbers>; do
  git fetch upstream "pull/${N}/head:refs/remotes/pr/${N}"
  TITLE=$(gh pr view "${N}" --repo malaterre/GDCM --json title --jq .title)
  git merge --no-ff "pr/${N}" -m "Merge malaterre/GDCM#${N}: ${TITLE}"
done
```

6. Replay the ITK build overlay from the previous fork branch:

```
PREV=for/itk-gdcm-master-4f20194
git fetch origin "${PREV}"
# The overlay commits are the non-merge commits after the last PR merge.
git log --oneline --first-parent --no-merges "$(git merge-base origin/${PREV} ${BASE})..origin/${PREV}"
git cherry-pick <oldest-overlay-sha>^..<newest-overlay-sha>
```

7. Name the branch after its tip and publish it:

```
SHA=$(git rev-parse --short=7 HEAD)
NEW_BRANCH="for/itk-gdcm-master-${SHA}"
git branch -m "${NEW_BRANCH}"
git push origin "${NEW_BRANCH}"
```

## Pin the new branch in ITK

8. In ITK, set `tag` in `Modules/ThirdParty/GDCM/UpdateFromUpstream.sh`:

```
readonly repo="https://github.com/InsightSoftwareConsortium/GDCM.git"
readonly tag="for/itk-gdcm-master-${SHA}"
```

   Commit that change, run `Modules/ThirdParty/GDCM/UpdateFromUpstream.sh` from
   a clean ITK checkout, and resolve any subtree-merge conflict in favor of this
   fork's version, so the vendored tree matches the fork exactly. Build ITK with
   `Module_ITKIOGDCM=ON` and confirm the GDCM and DICOM tests pass before
   opening the ITK pull request.
