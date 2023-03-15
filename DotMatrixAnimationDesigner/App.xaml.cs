using System;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows;

namespace DotMatrixAnimationDesigner
{
    public partial class App : Application
    {
        public App() : base()
        {
            AppDomain.CurrentDomain.UnhandledException += (s, e)
                => ShowUnhandledExceptionDialog((Exception)e.ExceptionObject, "AppDomain.CurrentDomain.UnhandledException");
            Dispatcher.UnhandledException += (s, e)
                => ShowUnhandledExceptionDialog(e.Exception, "Dispatcher.UnhandledException");
            TaskScheduler.UnobservedTaskException += (s, e)
                => ShowUnhandledExceptionDialog(e.Exception, "TaskScheduler.UnobservedTaskException");

            StartupUri = new Uri("MainWindow.xaml", UriKind.Relative);
        }

        private static void ShowUnhandledExceptionDialog(Exception e, string source)
        {
            AssemblyName assembly;
            var caption = "Unhandled exception occured";
            var msg = $"Source: {source}";

            try
            {
                assembly = Assembly.GetExecutingAssembly().GetName();
                msg += $"\n\nAssembly: '{assembly.Name}'\nVersion {assembly.Version}";
            }
            catch (Exception ex)
            {
                ErrorDialog("Could not get executing assembly", $"Exception information: '{ex.Message}'");
            }

            var current = e;
            var inner = e;
            while (inner != null)
            {
                current = inner;
                msg += $"\n\nException: {current?.GetType()}\nException information: '{current?.Message}'";
                inner = current?.InnerException;
            }

            msg += $"\n\nStack trace: {current?.StackTrace}";
            ErrorDialog(caption, msg);
        }

        private static void ErrorDialog(string caption, string msg)
        {
            _ = MessageBox.Show(caption, msg, MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }
}
