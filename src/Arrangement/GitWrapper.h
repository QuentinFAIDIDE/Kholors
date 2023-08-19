#ifndef DEF_GIT_WRAPPER_HPP
#define DEF_GIT_WRAPPER_HPP

#include "../Config.h"

#include <git2.h>
#include <git2/repository.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <stdexcept>
#include <string>

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
     * @brief      Gets the branch the repo on the working directory is on.
     *             Throw a runtime_error on unexpected git errors.
     *
     * @return     The branch.
     */
    std::string getBranch();

  private:
    std::string repositoryPath;
    git_repository *libgitRepo;

    juce::SharedResourcePointer<Config> sharedConfig;
};

#endif // DEF_GIT_WRAPPER_HPP