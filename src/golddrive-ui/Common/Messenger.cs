using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

//Messenger downloaded from StackOverflow and small modifications made
namespace golddrive
{
    public class Messenger
    {
        private static readonly object CreationLock = new object();
        private static readonly ConcurrentDictionary<MessengerKey, object> Dictionary = new ConcurrentDictionary<MessengerKey, object>();

        #region Default property

        private static Messenger _instance;

        /// <summary>
        /// Gets the single instance of the Messenger.
        /// </summary>
        public static Messenger Default
        {
            get
            {
                if (_instance == null)
                {
                    lock (CreationLock)
                    {
                        if (_instance == null)
                        {
                            _instance = new Messenger();
                        }
                    }
                }

                return _instance;
            }
        }

        #endregion

        /// <summary>
        /// Initializes a new instance of the Messenger class.
        /// </summary>
        private Messenger()
        {
        }

        /// <summary>
        /// Registers a recipient for a type of message T. The action parameter will be executed
        /// when a corresponding message is sent.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="recipient"></param>
        /// <param name="action"></param>
        public void Register<T>(object recipient, Action<T> action)
        {
            Register(recipient, action, null);
        }

        /// <summary>
        /// Registers a recipient for a type of message T and a matching context. The action parameter will be executed
        /// when a corresponding message is sent.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="recipient"></param>
        /// <param name="action"></param>
        /// <param name="context"></param>
        public void Register<T>(object recipient, Action<T> action, object context)
        {
            var key = new MessengerKey(recipient, context);
            Dictionary.TryAdd(key, action);
        }

        /// <summary>
        /// Unregisters a messenger recipient completely. After this method is executed, the recipient will
        /// no longer receive any messages.
        /// </summary>
        /// <param name="recipient"></param>
        public void Unregister(object recipient)
        {
            Unregister(recipient, null);
        }

        /// <summary>
        /// Unregisters a messenger recipient with a matching context completely. After this method is executed, the recipient will
        /// no longer receive any messages.
        /// </summary>
        /// <param name="recipient"></param>
        /// <param name="context"></param>
        public void Unregister(object recipient, object context)
        {
            object action;
            var key = new MessengerKey(recipient, context);
            Dictionary.TryRemove(key, out action);
        }

        /// <summary>
        /// Sends a message to registered recipients. The message will reach all recipients that are
        /// registered for this message type.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="message"></param>
        public void Send<T>(T message)
        {
            Send(message, null);
        }

        /// <summary>
        /// Sends a message to registered recipients. The message will reach all recipients that are
        /// registered for this message type and matching context.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="message"></param>
        /// <param name="context"></param>
        public void Send<T>(T message, object context)
        {
            IEnumerable<KeyValuePair<MessengerKey, object>> result;

            if (context == null)
            {
                // Get all recipients where the context is null.
                result = from r in Dictionary where r.Key.Context == null select r;
            }
            else
            {
                // Get all recipients where the context is matching.
                result = from r in Dictionary where r.Key.Context != null && r.Key.Context.Equals(context) select r;
            }

            foreach (var action in result.Select(x => x.Value).OfType<Action<T>>())
            {
                // Send the message to all recipients.
                action(message);
            }
        }

        protected class MessengerKey
        {
            public object Recipient { get; private set; }
            public object Context { get; private set; }

            /// <summary>
            /// Initializes a new instance of the MessengerKey class.
            /// </summary>
            /// <param name="recipient"></param>
            /// <param name="context"></param>
            public MessengerKey(object recipient, object context)
            {
                Recipient = recipient;
                Context = context;
            }

            /// <summary>
            /// Determines whether the specified MessengerKey is equal to the current MessengerKey.
            /// </summary>
            /// <param name="other"></param>
            /// <returns></returns>
            protected bool Equals(MessengerKey other)
            {
                return Equals(Recipient, other.Recipient) && Equals(Context, other.Context);
            }

            /// <summary>
            /// Determines whether the specified MessengerKey is equal to the current MessengerKey.
            /// </summary>
            /// <param name="obj"></param>
            /// <returns></returns>
            public override bool Equals(object obj)
            {
                if (ReferenceEquals(null, obj)) return false;
                if (ReferenceEquals(this, obj)) return true;
                if (obj.GetType() != GetType()) return false;

                return Equals((MessengerKey)obj);
            }

            /// <summary>
            /// Serves as a hash function for a particular type. 
            /// </summary>
            /// <returns></returns>
            public override int GetHashCode()
            {
                unchecked
                {
                    return ((Recipient != null ? Recipient.GetHashCode() : 0) * 397) ^ (Context != null ? Context.GetHashCode() : 0);
                }
            }
        }
    }

}

