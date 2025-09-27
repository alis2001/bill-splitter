import React, { useContext, useState } from 'react';
import { 
  View, Text, StyleSheet, TouchableOpacity, Alert, 
  ScrollView, Animated 
} from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { Ionicons } from '@expo/vector-icons';
import { AuthContext } from '../../context/AuthContext';

const ProfileScreen = () => {
  const { user, signOut } = useContext(AuthContext);
  const [scaleAnim] = useState(new Animated.Value(1));

  const handleLogout = () => {
    Alert.alert(
      'Sign Out',
      'Are you sure you want to sign out?',
      [
        { text: 'Cancel', style: 'cancel' },
        { 
          text: 'Sign Out', 
          style: 'destructive', 
          onPress: () => {
            Animated.sequence([
              Animated.timing(scaleAnim, {
                toValue: 0.95,
                duration: 100,
                useNativeDriver: true,
              }),
              Animated.timing(scaleAnim, {
                toValue: 1,
                duration: 100,
                useNativeDriver: true,
              })
            ]).start(() => signOut());
          }
        }
      ]
    );
  };

  const menuItems = [
    {
      icon: 'person-outline',
      title: 'Edit Profile',
      subtitle: 'Update your personal information',
      onPress: () => Alert.alert('Coming Soon', 'Profile editing will be available soon'),
      color: '#1A365D'
    },
    {
      icon: 'people-outline', 
      title: 'Manage Contacts',
      subtitle: 'Add friends and manage connections',
      onPress: () => Alert.alert('Coming Soon', 'Contact management will be available soon'),
      color: '#1A365D'
    },
    {
      icon: 'notifications-outline',
      title: 'Notifications',
      subtitle: 'Manage your notification preferences',
      onPress: () => Alert.alert('Coming Soon', 'Notification settings will be available soon'),
      color: '#1A365D'
    },
    {
      icon: 'help-circle-outline',
      title: 'Help & Support',
      subtitle: 'Get help and contact support',
      onPress: () => Alert.alert('Help & Support', 'For support, please email: support@billsplitter.com'),
      color: '#1A365D'
    },
    {
      icon: 'information-circle-outline',
      title: 'About',
      subtitle: 'App version and information',
      onPress: () => Alert.alert('Bill Splitter', 'Version 1.0.0\n\nMade with ❤️ for easy bill splitting'),
      color: '#1A365D'
    }
  ];

  const renderMenuItem = (item, index) => (
    <TouchableOpacity
      key={index}
      style={styles.menuItem}
      onPress={item.onPress}
      activeOpacity={0.7}
    >
      <View style={[styles.menuIcon, { backgroundColor: `${item.color}10` }]}>
        <Ionicons name={item.icon} size={20} color={item.color} />
      </View>
      <View style={styles.menuContent}>
        <Text style={styles.menuTitle}>{item.title}</Text>
        <Text style={styles.menuSubtitle}>{item.subtitle}</Text>
      </View>
      <Ionicons name="chevron-forward" size={16} color="#A0AEC0" />
    </TouchableOpacity>
  );

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView showsVerticalScrollIndicator={false}>
        <View style={styles.header}>
          <Text style={styles.title}>Profile</Text>
        </View>
        
        {/* Profile Card */}
        <Animated.View style={[styles.profileCard, { transform: [{ scale: scaleAnim }] }]}>
          <View style={styles.avatarContainer}>
            <View style={styles.avatar}>
              <Text style={styles.avatarText}>
                {user?.name?.charAt(0)?.toUpperCase()}{user?.family_name?.charAt(0)?.toUpperCase()}
              </Text>
            </View>
            <View style={styles.onlineIndicator} />
          </View>
          
          <View style={styles.userInfo}>
            <Text style={styles.userName}>
              {user?.name} {user?.family_name}
            </Text>
            <Text style={styles.userEmail}>{user?.email}</Text>
            <View style={styles.joinedInfo}>
              <Ionicons name="calendar-outline" size={14} color="#718096" />
              <Text style={styles.joinedText}>
                Joined {new Date(user?.created_at).toLocaleDateString('en-US', { 
                  month: 'long', 
                  year: 'numeric' 
                })}
              </Text>
            </View>
          </View>
        </Animated.View>

        {/* Menu Items */}
        <View style={styles.menuSection}>
          {menuItems.map(renderMenuItem)}
        </View>

        {/* Logout Button */}
        <TouchableOpacity 
          style={styles.logoutButton} 
          onPress={handleLogout}
          activeOpacity={0.8}
        >
          <Ionicons name="log-out-outline" size={20} color="#E53E3E" />
          <Text style={styles.logoutText}>Sign Out</Text>
        </TouchableOpacity>

        {/* Footer */}
        <View style={styles.footer}>
          <Text style={styles.footerText}>Bill Splitter v1.0.0</Text>
          <Text style={styles.footerSubtext}>Made with ❤️ for splitting bills easily</Text>
        </View>
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#FAFAFA',
  },
  header: {
    paddingHorizontal: 16,
    paddingVertical: 16,
    backgroundColor: '#FFFFFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E2E8F0',
  },
  title: {
    fontSize: 24,
    fontWeight: '700',
    color: '#2D3748',
  },
  profileCard: {
    backgroundColor: '#FFFFFF',
    borderRadius: 16,
    padding: 24,
    margin: 16,
    alignItems: 'center',
    elevation: 3,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
  },
  avatarContainer: {
    position: 'relative',
    marginBottom: 16,
  },
  avatar: {
    width: 80,
    height: 80,
    borderRadius: 40,
    backgroundColor: '#1A365D',
    justifyContent: 'center',
    alignItems: 'center',
  },
  avatarText: {
    fontSize: 24,
    fontWeight: '700',
    color: '#FFFFFF',
  },
  onlineIndicator: {
    position: 'absolute',
    bottom: 2,
    right: 2,
    width: 20,
    height: 20,
    borderRadius: 10,
    backgroundColor: '#38A169',
    borderWidth: 3,
    borderColor: '#FFFFFF',
  },
  userInfo: {
    alignItems: 'center',
  },
  userName: {
    fontSize: 20,
    fontWeight: '600',
    color: '#2D3748',
    marginBottom: 4,
  },
  userEmail: {
    fontSize: 14,
    color: '#718096',
    marginBottom: 8,
  },
  joinedInfo: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  joinedText: {
    fontSize: 12,
    color: '#718096',
    marginLeft: 4,
  },
  menuSection: {
    marginHorizontal: 16,
    marginBottom: 24,
  },
  menuItem: {
    backgroundColor: '#FFFFFF',
    borderRadius: 12,
    padding: 16,
    marginBottom: 8,
    flexDirection: 'row',
    alignItems: 'center',
    elevation: 1,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
  },
  menuIcon: {
    width: 40,
    height: 40,
    borderRadius: 20,
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 12,
  },
  menuContent: {
    flex: 1,
  },
  menuTitle: {
    fontSize: 16,
    fontWeight: '500',
    color: '#2D3748',
    marginBottom: 2,
  },
  menuSubtitle: {
    fontSize: 12,
    color: '#718096',
  },
  logoutButton: {
    backgroundColor: '#FFFFFF',
    borderRadius: 12,
    padding: 16,
    marginHorizontal: 16,
    marginBottom: 24,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    elevation: 1,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
    borderWidth: 1,
    borderColor: '#FED7D7',
  },
  logoutText: {
    fontSize: 16,
    fontWeight: '600',
    color: '#E53E3E',
    marginLeft: 8,
  },
  footer: {
    alignItems: 'center',
    paddingBottom: 32,
  },
  footerText: {
    fontSize: 12,
    color: '#A0AEC0',
    marginBottom: 4,
  },
  footerSubtext: {
    fontSize: 10,
    color: '#A0AEC0',
  },
});

export default ProfileScreen;