#ifndef DEF_GIT_WRAPPER_HPP
#define DEF_GIT_WRAPPER_HPP

#include "../Config.h"

#include <git2.h>
#include <git2/repository.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <stdexcept>
#include <string>

struct RepositoryStatistics
{
    std::string name;
    int noCommits;
    int noBranches;
    std::time_t firstCommitDate;
    std::time_t lastCommitDate;
};

/**
 * @brief      This class describes a git wrapper.
 *             It will initialize git repositories, add file
 *             commit them, switch branches, and eventually
 *             push to a remote one day.
 */
class GitWrapper
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    GitWrapper();

    /**
     * @brief      Destroys the object.
     */
    ~GitWrapper();

    /**
     * @brief      Sets the working directory.
     *
     * @param[in]  path  The path
     */
    void setWorkingDirectory(std::string path);

    /**
     * @brief      Initializes the git repository in this working
     *             directory. (similar to git init).
     */
    void init();

    /**
     * @brief      Open git repository at current path.
     *
     */
    void open();

    /**
     * @brief      Add a file to the current commit (similar to git add).
     *
     * @param[in]  filenameRelativeFromDirectory  The filename relative from directory
     */
    void add(std::string filenameRelativeFromDirectory);

    /**
     * @brief      Commit files added. Similar to a git commit.
     *
     * @param[in]  commitMessage  The commit message
     *
     */
    void commit(std::string commitMessage);

    /**
     * @brief      Make a git hard reset to HEAD, overwriting current changes
     *             for tracked files with the git index (content) at the last commit.
     */
    void resetCurrentChanges();

    /**
     * @brief      Gets the branch the repo on the working directory is on.
     *             Throw a runtime_error on unexpected git errors.
     *
     * @return     The branch.
     */
    std::string getBranch();

    /**
     * @brief      Gets repository statistics for the repo in the folder at provided path.
     *
     * @param[in]  pathToRepository  The path on disk to the repository.
     *
     * @return     A structure describing various statistics like commit counts and last modification date.
     */
    RepositoryStatistics getRepositoryStatistics(std::string pathToRepository);

  private:
    std::string repositoryPath;
    git_repository *libgitRepo;

    juce::SharedResourcePointer<Config> sharedConfig;

    /**
     * @brief      Walks all commits and records their count, as well as first and last timestamp.
     *
     * @param      repo                    The repository already opened (libgit2 object)
     * @param      destinationStatsStruct  The destination statistics structure
     */
    void recordRepoCommitStats(git_repository *repo, RepositoryStatistics &destinationStatsStruct);

    /**
     * @brief      Counts the number of branches in the repository.
     *
     * @param      repo                    The repository already opened (libgit2 object)
     * @param      destinationStatsStruct  The destination statistics structure
     */
    void countRepoBranches(git_repository *repo, RepositoryStatistics &destinationStatsStruct);
};

#endif // DEF_GIT_WRAPPER_HPP