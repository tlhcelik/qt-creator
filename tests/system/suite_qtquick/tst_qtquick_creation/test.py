source("../../shared/qtcreator.py")

refreshFinishedCount = 0
workingDir = None

def handleRefreshFinished(object, fileList):
    global refreshFinishedCount
    refreshFinishedCount += 1

def main():
    global workingDir,buildFinished,buildSucceeded
    startApplication("qtcreator" + SettingsPath)
    installLazySignalHandler("{type='CppTools::Internal::CppModelManager'}", "sourceFilesRefreshed(QStringList)", "handleRefreshFinished")
    # using a temporary directory won't mess up an eventually exisiting
    workingDir = tempDir()
    createNewQtQuickApplication()
    # wait for parsing to complete
    waitFor("refreshFinishedCount == 1", 10000)
    test.log("Building project")
    invokeMenuItem("Build","Build All")
    waitForBuildFinished()
    test.log("Running project (includes build)")
    if runAndCloseApp():
        logApplicationOutput()
    invokeMenuItem("File", "Exit")

def createNewQtQuickApplication():
    global workingDir
    invokeMenuItem("File", "New File or Project...")
    clickItem(waitForObject("{type='QTreeView' name='templateCategoryView'}", 20000), "Projects.Qt Quick Project", 5, 5, 0, Qt.LeftButton)
    clickItem(waitForObject("{name='templatesView' type='QListView'}", 20000), "Qt Quick Application", 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}", 20000))
    baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}", 20000)
    replaceLineEditorContent(baseLineEd, workingDir)
    stateLabel = findObject("{type='QLabel' name='stateLabel'}")
    labelCheck = stateLabel.text=="" and stateLabel.styleSheet == ""
    test.verify(labelCheck, "Project name and base directory without warning or error")
    # make sure this is not set as default location
    cbDefaultLocation = waitForObject("{type='QCheckBox' name='projectsDirectoryCheckBox' visible='1'}", 20000)
    if cbDefaultLocation.checked:
        clickButton(cbDefaultLocation)
    # now there's the 'untitled' project inside a temporary directory - step forward...!
    nextButton = waitForObject("{text='Next' type='QPushButton' visible='1'}", 20000)
    clickButton(nextButton)
    chooseComponents()
    clickButton(nextButton)
    chooseDestination(QtQuickConstants.Destinations.DESKTOP)
    snooze(1)
    clickButton(nextButton)
    clickButton(waitForObject("{type='QPushButton' text='Finish' visible='1'}", 20000))

def cleanup():
    global workingDir
    # waiting for a clean exit - for a full-remove of the temp directory
    appCtxt = currentApplicationContext()
    waitFor("appCtxt.isRunning==False")
    if workingDir!=None:
        deleteDirIfExists(workingDir)
