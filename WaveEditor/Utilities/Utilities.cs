using EnvDTE;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Threading;

namespace WaveEditor.Utilities {
    public static class ID {
        public static int INVALID_ID => -1;
        public static bool IsValid(int id) => id != INVALID_ID;
    }

    public static class Utilities {
        public static float Epsilon => 0.00001f;
        public static bool IsTheSameAs(this float value, float other) {
            return Math.Abs(value - other) < Epsilon;
        }
        
        public static bool IsTheSameAs(this float? value, float? other) {
            if (!value.HasValue || !other.HasValue) return false;
            return Math.Abs(value.Value - other.Value) < Epsilon;
        }
    }

    class DelayEventTimerArgs : EventArgs {
        public bool RepeatEvent { get; set; }
        public object Data { get; set; }

        public DelayEventTimerArgs(object data) {
            Data = data;
        }
    }

    class DelayEventTimer {
        private readonly DispatcherTimer _timer;
        private readonly TimeSpan _delay;
        private DateTime _lastEventTime = DateTime.Now;
        private object _data;

        public event EventHandler<DelayEventTimerArgs> Triggered;

        // when called(mouse pressed), record time point into _lastEventTime
        // and this function kept being called unless the left mouse button is released.
        // Thus, the _lastEventTime is refreshed all the time unless the left mouse button is released.
        public void Trigger(object data = null) {
            _data = data;
            _lastEventTime = DateTime.Now; 
            _timer.IsEnabled = true; // launch the timer
        }

        public void Disable() {
            _timer.IsEnabled = false;
        }

        // once enabled, OnTimerTick will be called after every time interval
        private void OnTimerTick(object sender, EventArgs e) {
            if ((DateTime.Now - _lastEventTime) < _delay) return;
            var eventArgs = new DelayEventTimerArgs(_data);
            Triggered?.Invoke(this, eventArgs); // set eventArgs.RepeatEvent to whether the mouse button is still pressed
            _timer.IsEnabled = eventArgs.RepeatEvent; // false -> stop ticking
        }

        public DelayEventTimer(TimeSpan delay, DispatcherPriority priority = DispatcherPriority.Normal) {
            _delay = delay;
            _timer = new DispatcherTimer(priority) {
                Interval = TimeSpan.FromMilliseconds(delay.TotalMilliseconds * 0.5)
            };
            _timer.Tick += OnTimerTick;
        }
    }
}
